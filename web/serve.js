#!/usr/bin/env bun
/**
 * Simple static file server for testing
 * Usage: bun run serve.js
 */

const port = 3000;

Bun.serve({
  port,
  fetch(request) {
    const url = new URL(request.url);
    let path = url.pathname;

    // Default to test.html
    if (path === '/') {
      path = '/test.html';
    }

    // Resolve file path
    const filePath = `${import.meta.dir}${path}`;

    try {
      const file = Bun.file(filePath);
      return new Response(file);
    } catch (e) {
      return new Response('Not found', { status: 404 });
    }
  },
});

console.log(`Static server running at http://localhost:${port}`);
console.log('Open http://localhost:3000 in your browser');
console.log('Press Ctrl+C to stop');
