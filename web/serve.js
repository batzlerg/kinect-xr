#!/usr/bin/env bun
/**
 * Simple static file server for testing
 * Usage: bun run serve.js
 */

const port = 3000;

Bun.serve({
  port,
  async fetch(request) {
    const url = new URL(request.url);
    let path = url.pathname;

    // Default to index.html (examples landing page)
    if (path === '/') {
      path = '/index.html';
    }

    // Resolve file path
    let filePath = `${import.meta.dir}${path}`;

    try {
      // Check if path is a directory
      const stat = await Bun.file(filePath).exists();

      if (stat) {
        // If it's a directory path (ends with /), try index.html
        if (path.endsWith('/')) {
          filePath = `${import.meta.dir}${path}index.html`;
        }
      }

      const file = Bun.file(filePath);

      // Check if file exists and is a regular file
      if (await file.exists()) {
        return new Response(file);
      }

      // If path doesn't end with / but is a directory, try adding index.html
      const indexPath = `${filePath}/index.html`;
      const indexFile = Bun.file(indexPath);
      if (await indexFile.exists()) {
        return new Response(indexFile);
      }

      return new Response('Not found', { status: 404 });
    } catch (e) {
      console.error('Error serving file:', e);
      return new Response('Server error', { status: 500 });
    }
  },
});

console.log(`Static server running at http://localhost:${port}`);
console.log('Open http://localhost:3000 in your browser');
console.log('Press Ctrl+C to stop');
