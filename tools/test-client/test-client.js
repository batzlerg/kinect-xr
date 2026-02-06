#!/usr/bin/env node
/**
 * Kinect XR WebSocket Bridge Test Client
 *
 * Validates the protocol defined in specs/008-WebSocketBridgeProtocol.md
 *
 * Usage:
 *   node test-client.js              # Connect and display frames
 *   node test-client.js --benchmark  # Run 60-second benchmark
 *   node test-client.js --rgb-only   # Subscribe to RGB only
 *   node test-client.js --depth-only # Subscribe to depth only
 */

import WebSocket from 'ws';

// Configuration
const WS_URL = process.env.KINECT_BRIDGE_URL || 'ws://localhost:8765/kinect';
const BENCHMARK_DURATION_MS = 60_000;

// Protocol constants
const STREAM_TYPE_RGB = 0x0001;
const STREAM_TYPE_DEPTH = 0x0002;
const RGB_FRAME_SIZE = 640 * 480 * 3;  // 921600 bytes
const DEPTH_FRAME_SIZE = 640 * 480 * 2; // 614400 bytes

// Parse arguments
const args = process.argv.slice(2);
const benchmarkMode = args.includes('--benchmark');
const rgbOnly = args.includes('--rgb-only');
const depthOnly = args.includes('--depth-only');

// Determine streams to subscribe
let streams = ['rgb', 'depth'];
if (rgbOnly) streams = ['rgb'];
if (depthOnly) streams = ['depth'];

// Stats tracking
const stats = {
  connected: false,
  startTime: null,
  rgbFrames: 0,
  depthFrames: 0,
  lastRgbFrameId: -1,
  lastDepthFrameId: -1,
  droppedRgbFrames: 0,
  droppedDepthFrames: 0,
  bytesReceived: 0,
  errors: [],
  capabilities: null,
};

function formatBytes(bytes) {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
}

function printStats() {
  const elapsed = (Date.now() - stats.startTime) / 1000;
  const rgbFps = stats.rgbFrames / elapsed;
  const depthFps = stats.depthFrames / elapsed;
  const bandwidth = stats.bytesReceived / elapsed;

  console.log('\n--- Statistics ---');
  console.log(`Duration: ${elapsed.toFixed(1)}s`);
  console.log(`RGB frames: ${stats.rgbFrames} (${rgbFps.toFixed(1)} fps, ${stats.droppedRgbFrames} dropped)`);
  console.log(`Depth frames: ${stats.depthFrames} (${depthFps.toFixed(1)} fps, ${stats.droppedDepthFrames} dropped)`);
  console.log(`Bandwidth: ${formatBytes(bandwidth)}/s`);
  console.log(`Total received: ${formatBytes(stats.bytesReceived)}`);
  if (stats.errors.length > 0) {
    console.log(`Errors: ${stats.errors.length}`);
    stats.errors.slice(-5).forEach(e => console.log(`  - ${e}`));
  }
}

function handleJsonMessage(msg) {
  switch (msg.type) {
    case 'hello':
      console.log('Received hello from server');
      console.log(`  Protocol version: ${msg.protocol_version}`);
      console.log(`  Server: ${msg.server}`);
      console.log(`  Streams: ${msg.capabilities.streams.join(', ')}`);
      console.log(`  RGB: ${msg.capabilities.rgb.width}x${msg.capabilities.rgb.height} ${msg.capabilities.rgb.format}`);
      console.log(`  Depth: ${msg.capabilities.depth.width}x${msg.capabilities.depth.height} ${msg.capabilities.depth.format}`);
      console.log(`  Frame rate: ${msg.capabilities.frame_rate_hz} Hz`);
      stats.capabilities = msg.capabilities;

      // Validate capabilities
      if (msg.capabilities.rgb.bytes_per_frame !== RGB_FRAME_SIZE) {
        stats.errors.push(`Unexpected RGB frame size: ${msg.capabilities.rgb.bytes_per_frame}`);
      }
      if (msg.capabilities.depth.bytes_per_frame !== DEPTH_FRAME_SIZE) {
        stats.errors.push(`Unexpected depth frame size: ${msg.capabilities.depth.bytes_per_frame}`);
      }
      break;

    case 'status':
      if (!benchmarkMode) {
        console.log(`Status: kinect=${msg.kinect_connected}, frame=${msg.frame_id}, dropped=${msg.dropped_frames}, clients=${msg.clients_connected}`);
      }
      break;

    case 'error':
      console.error(`Error [${msg.code}]: ${msg.message} (recoverable: ${msg.recoverable})`);
      stats.errors.push(`${msg.code}: ${msg.message}`);
      break;

    case 'goodbye':
      console.log(`Server goodbye: ${msg.reason}`);
      break;

    default:
      console.log(`Unknown message type: ${msg.type}`);
  }
}

function handleBinaryMessage(data) {
  if (data.byteLength < 8) {
    stats.errors.push(`Binary frame too small: ${data.byteLength} bytes`);
    return;
  }

  const header = new DataView(data.buffer, data.byteOffset, 8);
  const frameId = header.getUint32(0, true);
  const streamType = header.getUint16(4, true);
  const reserved = header.getUint16(6, true);
  const payloadSize = data.byteLength - 8;

  stats.bytesReceived += data.byteLength;

  if (reserved !== 0) {
    stats.errors.push(`Non-zero reserved field: ${reserved}`);
  }

  if (streamType === STREAM_TYPE_RGB) {
    if (payloadSize !== RGB_FRAME_SIZE) {
      stats.errors.push(`RGB frame wrong size: ${payloadSize} (expected ${RGB_FRAME_SIZE})`);
      return;
    }

    // Check for dropped frames
    if (stats.lastRgbFrameId >= 0 && frameId !== stats.lastRgbFrameId + 1) {
      const dropped = frameId - stats.lastRgbFrameId - 1;
      if (dropped > 0) {
        stats.droppedRgbFrames += dropped;
      }
    }
    stats.lastRgbFrameId = frameId;
    stats.rgbFrames++;

    if (!benchmarkMode && stats.rgbFrames % 30 === 0) {
      console.log(`RGB frame ${frameId} (${stats.rgbFrames} total)`);
    }

  } else if (streamType === STREAM_TYPE_DEPTH) {
    if (payloadSize !== DEPTH_FRAME_SIZE) {
      stats.errors.push(`Depth frame wrong size: ${payloadSize} (expected ${DEPTH_FRAME_SIZE})`);
      return;
    }

    // Check for dropped frames
    if (stats.lastDepthFrameId >= 0 && frameId !== stats.lastDepthFrameId + 1) {
      const dropped = frameId - stats.lastDepthFrameId - 1;
      if (dropped > 0) {
        stats.droppedDepthFrames += dropped;
      }
    }
    stats.lastDepthFrameId = frameId;
    stats.depthFrames++;

    if (!benchmarkMode && stats.depthFrames % 30 === 0) {
      // Sample some depth values
      const depthData = new Uint16Array(data.buffer, data.byteOffset + 8);
      const centerIdx = 320 + 240 * 640; // Center pixel
      const centerDepth = depthData[centerIdx];
      console.log(`Depth frame ${frameId} (center depth: ${centerDepth}mm)`);
    }

  } else {
    stats.errors.push(`Unknown stream type: ${streamType}`);
  }
}

function connect() {
  console.log(`Connecting to ${WS_URL}...`);
  const ws = new WebSocket(WS_URL);

  ws.on('open', () => {
    console.log('Connected!');
    stats.connected = true;
    stats.startTime = Date.now();
  });

  ws.on('message', (data, isBinary) => {
    if (isBinary) {
      handleBinaryMessage(data);
    } else {
      try {
        const msg = JSON.parse(data.toString());
        handleJsonMessage(msg);

        // Send subscribe after receiving hello
        if (msg.type === 'hello') {
          console.log(`\nSubscribing to: ${streams.join(', ')}`);
          ws.send(JSON.stringify({ type: 'subscribe', streams }));

          if (benchmarkMode) {
            console.log(`\nRunning benchmark for ${BENCHMARK_DURATION_MS / 1000} seconds...`);
            setTimeout(() => {
              console.log('\nBenchmark complete.');
              printStats();
              ws.send(JSON.stringify({ type: 'unsubscribe' }));
              ws.close();
              process.exit(0);
            }, BENCHMARK_DURATION_MS);
          }
        }
      } catch (e) {
        stats.errors.push(`JSON parse error: ${e.message}`);
      }
    }
  });

  ws.on('error', (error) => {
    console.error('WebSocket error:', error.message);
    stats.errors.push(error.message);
  });

  ws.on('close', (code, reason) => {
    console.log(`Connection closed: ${code} ${reason}`);
    stats.connected = false;
    printStats();
  });

  // Handle Ctrl+C
  process.on('SIGINT', () => {
    console.log('\nInterrupted.');
    printStats();
    ws.close();
    process.exit(0);
  });
}

// Main
console.log('Kinect XR WebSocket Bridge Test Client');
console.log('======================================');
console.log(`Mode: ${benchmarkMode ? 'Benchmark' : 'Interactive'}`);
console.log(`Streams: ${streams.join(', ')}`);
console.log('');

connect();
