/**
 * Kinect XR WebSocket Client
 *
 * Simple browser library for receiving Kinect RGB + depth data.
 *
 * Usage:
 *   import { KinectClient } from './kinect-client.js';
 *
 *   const kinect = new KinectClient();
 *
 *   kinect.onRgbFrame = (imageData, frameId) => {
 *     ctx.putImageData(imageData, 0, 0);
 *   };
 *
 *   kinect.onDepthFrame = (depthArray, frameId) => {
 *     // depthArray is Uint16Array[307200] (640x480)
 *     // values are depth in mm (800-4000)
 *   };
 *
 *   await kinect.connect();
 */

// Protocol constants
const STREAM_TYPE_RGB = 0x0001;
const STREAM_TYPE_DEPTH = 0x0002;
const FRAME_WIDTH = 640;
const FRAME_HEIGHT = 480;
const RGB_FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 3;
const DEPTH_FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 2;

/**
 * Kinect WebSocket client for browser
 */
export class KinectClient {
  /**
   * Create a KinectClient
   * @param {string} url - WebSocket URL (default: ws://localhost:8765/kinect)
   */
  constructor(url = 'ws://localhost:8765/kinect') {
    this.url = url;
    this.ws = null;
    this.connected = false;
    this.capabilities = null;

    // User callbacks
    this.onRgbFrame = null;    // (imageData: ImageData, frameId: number) => void
    this.onDepthFrame = null;  // (depth: Uint16Array, frameId: number) => void
    this.onConnect = null;     // (capabilities: object) => void
    this.onDisconnect = null;  // () => void
    this.onError = null;       // (error: object) => void
    this.onStatus = null;      // (status: object) => void

    // Statistics
    this.stats = {
      rgbFrames: 0,
      depthFrames: 0,
      lastRgbFrameId: -1,
      lastDepthFrameId: -1,
      droppedRgbFrames: 0,
      droppedDepthFrames: 0,
      bytesReceived: 0,
    };

    // Streams to subscribe to
    this._streams = ['rgb', 'depth'];

    // Temporary buffer for RGB conversion
    this._rgbaBuffer = new Uint8ClampedArray(FRAME_WIDTH * FRAME_HEIGHT * 4);
  }

  /**
   * Set which streams to subscribe to
   * @param {string[]} streams - Array of stream names ('rgb', 'depth')
   */
  setStreams(streams) {
    this._streams = streams;
    if (this.connected) {
      this._subscribe();
    }
  }

  /**
   * Connect to the Kinect bridge server
   * @returns {Promise<object>} Resolves with capabilities on success
   */
  connect() {
    return new Promise((resolve, reject) => {
      if (this.connected) {
        resolve(this.capabilities);
        return;
      }

      this.ws = new WebSocket(this.url);
      this.ws.binaryType = 'arraybuffer';

      this.ws.onopen = () => {
        console.log('[KinectClient] Connected to', this.url);
      };

      this.ws.onmessage = (event) => {
        if (typeof event.data === 'string') {
          this._handleJsonMessage(event.data, resolve, reject);
        } else {
          this._handleBinaryFrame(event.data);
        }
      };

      this.ws.onerror = (error) => {
        console.error('[KinectClient] WebSocket error:', error);
        if (this.onError) {
          this.onError({ type: 'websocket', message: 'Connection error' });
        }
        reject(new Error('WebSocket connection failed'));
      };

      this.ws.onclose = () => {
        console.log('[KinectClient] Disconnected');
        this.connected = false;
        if (this.onDisconnect) {
          this.onDisconnect();
        }
      };
    });
  }

  /**
   * Disconnect from the server
   */
  disconnect() {
    if (this.ws) {
      this.ws.close();
      this.ws = null;
      this.connected = false;
    }
  }

  /**
   * Get current statistics
   * @returns {object} Statistics object
   */
  getStats() {
    return { ...this.stats };
  }

  /**
   * Reset statistics
   */
  resetStats() {
    this.stats = {
      rgbFrames: 0,
      depthFrames: 0,
      lastRgbFrameId: -1,
      lastDepthFrameId: -1,
      droppedRgbFrames: 0,
      droppedDepthFrames: 0,
      bytesReceived: 0,
    };
  }

  // Private methods

  _handleJsonMessage(data, connectResolve, connectReject) {
    try {
      const msg = JSON.parse(data);

      switch (msg.type) {
        case 'hello':
          console.log('[KinectClient] Received hello:', msg.server, 'v' + msg.protocol_version);
          this.capabilities = msg.capabilities;
          this.connected = true;
          this._subscribe();
          if (this.onConnect) {
            this.onConnect(this.capabilities);
          }
          if (connectResolve) {
            connectResolve(this.capabilities);
          }
          break;

        case 'status':
          if (this.onStatus) {
            this.onStatus(msg);
          }
          break;

        case 'error':
          console.error('[KinectClient] Server error:', msg.code, msg.message);
          if (this.onError) {
            this.onError(msg);
          }
          if (!this.connected && connectReject) {
            connectReject(new Error(msg.message));
          }
          break;

        case 'goodbye':
          console.log('[KinectClient] Server goodbye:', msg.reason);
          break;

        default:
          console.warn('[KinectClient] Unknown message type:', msg.type);
      }
    } catch (e) {
      console.error('[KinectClient] Failed to parse JSON:', e);
    }
  }

  _handleBinaryFrame(buffer) {
    if (buffer.byteLength < 8) {
      console.warn('[KinectClient] Binary frame too small:', buffer.byteLength);
      return;
    }

    const header = new DataView(buffer, 0, 8);
    const frameId = header.getUint32(0, true);
    const streamType = header.getUint16(4, true);
    const payloadSize = buffer.byteLength - 8;

    this.stats.bytesReceived += buffer.byteLength;

    if (streamType === STREAM_TYPE_RGB) {
      if (payloadSize !== RGB_FRAME_SIZE) {
        console.warn('[KinectClient] RGB frame wrong size:', payloadSize);
        return;
      }

      // Track dropped frames
      if (this.stats.lastRgbFrameId >= 0 && frameId > this.stats.lastRgbFrameId + 1) {
        this.stats.droppedRgbFrames += frameId - this.stats.lastRgbFrameId - 1;
      }
      this.stats.lastRgbFrameId = frameId;
      this.stats.rgbFrames++;

      if (this.onRgbFrame) {
        // Convert RGB888 to RGBA for ImageData
        const rgb = new Uint8Array(buffer, 8);
        this._convertRgbToRgba(rgb);
        const imageData = new ImageData(this._rgbaBuffer, FRAME_WIDTH, FRAME_HEIGHT);
        this.onRgbFrame(imageData, frameId);
      }

    } else if (streamType === STREAM_TYPE_DEPTH) {
      if (payloadSize !== DEPTH_FRAME_SIZE) {
        console.warn('[KinectClient] Depth frame wrong size:', payloadSize);
        return;
      }

      // Track dropped frames
      if (this.stats.lastDepthFrameId >= 0 && frameId > this.stats.lastDepthFrameId + 1) {
        this.stats.droppedDepthFrames += frameId - this.stats.lastDepthFrameId - 1;
      }
      this.stats.lastDepthFrameId = frameId;
      this.stats.depthFrames++;

      if (this.onDepthFrame) {
        const depth = new Uint16Array(buffer, 8);
        this.onDepthFrame(depth, frameId);
      }

    } else {
      console.warn('[KinectClient] Unknown stream type:', streamType);
    }
  }

  _subscribe() {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify({
        type: 'subscribe',
        streams: this._streams
      }));
      console.log('[KinectClient] Subscribed to:', this._streams.join(', '));
    }
  }

  _convertRgbToRgba(rgb) {
    // Convert RGB888 to RGBA8888 for canvas ImageData
    for (let i = 0, j = 0; i < rgb.length; i += 3, j += 4) {
      this._rgbaBuffer[j] = rgb[i];         // R
      this._rgbaBuffer[j + 1] = rgb[i + 1]; // G
      this._rgbaBuffer[j + 2] = rgb[i + 2]; // B
      this._rgbaBuffer[j + 3] = 255;        // A
    }
  }
}

// Export constants for convenience
export const KINECT = {
  WIDTH: FRAME_WIDTH,
  HEIGHT: FRAME_HEIGHT,
  STREAM_RGB: 'rgb',
  STREAM_DEPTH: 'depth',
  MIN_DEPTH_MM: 800,
  MAX_DEPTH_MM: 4000,
};

export default KinectClient;
