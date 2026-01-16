const http = require('http');
const fs = require('fs');
const path = require('path');
const url = require('url');

// Parse command line arguments
const args = process.argv.slice(2);
let directory = '.';
let port = 8080;

for (let i = 0; i < args.length; i++) {
  if (args[i] === '-d' && i + 1 < args.length) {
    directory = args[i + 1];
    i++;
  } else if (args[i] === '-p' && i + 1 < args.length) {
    port = parseInt(args[i + 1]);
    i++;
  }
}

const MIME_TYPES = {
  '.html': 'text/html',
  '.js': 'application/javascript',
  '.wasm': 'application/wasm',
  '.css': 'text/css',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.gif': 'image/gif',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon'
};

const server = http.createServer((req, res) => {
  const parsedUrl = url.parse(req.url);
  let pathname = parsedUrl.pathname;

  // Default to index.html for root
  if (pathname === '/') {
    // List .html files in directory
    const files = fs.readdirSync(directory);
    const htmlFiles = files.filter(f => f.endsWith('.html'));

    if (htmlFiles.length > 0) {
      pathname = '/' + htmlFiles[0];
    } else {
      pathname = '/index.html';
    }
  }

  const filePath = path.join(directory, pathname);
  const ext = path.extname(filePath).toLowerCase();
  const contentType = MIME_TYPES[ext] || 'application/octet-stream';

  fs.readFile(filePath, (err, data) => {
    if (err) {
      res.writeHead(404, { 'Content-Type': 'text/plain' });
      res.end('404 Not Found');
      return;
    }

    // Required headers for SharedArrayBuffer and pthreads
    res.writeHead(200, {
      'Content-Type': contentType,
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp',
      'Cache-Control': 'no-cache'
    });

    res.end(data);
  });
});

server.listen(port, () => {
  console.log(`Server running at http://localhost:${port}/`);
  console.log(`Serving files from: ${path.resolve(directory)}`);
});