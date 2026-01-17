const http = require('http');
const filesystem = require('fs');
const path = require('path');
const port = 8080;

const arguments = process.argv.slice(2);
let dir = '.';
let indexFile = 'index.html';

for (let index = 0; index < arguments.length; index++) {
  if (arguments[index] === '-d') dir = arguments[++index];
  if (arguments[index] === '-f') indexFile = arguments[++index];
}

const MIME = {
  '.js': 'application/javascript',
  '.wasm': 'application/wasm',
  '.html': 'text/html'
};

http.createServer((request, result) => {
  const file = path.join(dir, request.url === '/' ? indexFile : request.url);
  const ext = path.extname(file);

  filesystem.readFile(file, (error, data) => {
    if (error) {
      result.writeHead(404);
      result.end('404 Not Found');
      return;
    }

    result.writeHead(200, {
      'Content-Type': MIME[ext] || 'application/octet-stream',
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp'
    });
    result.end(data);
  });
}).listen(port, () => console.log(`http://localhost:${port}/`));