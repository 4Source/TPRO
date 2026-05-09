import { defineConfig, Plugin } from 'vite';
import preact from '@preact/preset-vite';

// Dev-only plugin: intercepts effect writes/deletes and stores them in memory.
// Augments GET /directory/* and GET /file/* responses so saved effects appear
// in the UI without needing the real ESP backend.
function devMockWriteHandler(): Plugin {
	// "/effects/my_x.json" → JSON body
	const store = new Map<string, string>();

	return {
		name: 'dev-mock-write-handler',
		configureServer(server) {
			server.middlewares.use((req: any, res: any, next: () => void) => {
				const url: string = req.url ?? '';

				// PUT /effect/<path> → store body in memory
				if (req.method === 'PUT' && url.startsWith('/effect/')) {
					// e.g. /effects/my_x.json
					const filePath = url.slice('/effect'.length);
					let body = '';
					req.on('data', (chunk: unknown) => { body += String(chunk); });
					req.on('end', () => {
						store.set(filePath, body);
						res.writeHead(200, { 'Content-Type': 'text/plain' });
						res.end('OK');
					});
					return;
				}

				// DELETE /file/<path> → remove from memory
				if (req.method === 'DELETE' && url.startsWith('/file/')) {
					const filePath = url.slice('/file'.length);
					store.delete(filePath);
					req.resume();
					req.on('end', () => {
						res.writeHead(200, { 'Content-Type': 'text/plain' });
						res.end('OK');
					});
					return;
				}

				// GET /file/<path> → serve from memory if present (skip proxy)
				if (req.method === 'GET' && url.startsWith('/file/')) {
					const filePath = url.slice('/file'.length);
					const saved = store.get(filePath);
					if (saved !== undefined) {
						res.writeHead(200, { 'Content-Type': 'application/json' });
						res.end(saved);
						return;
					}
				}

				// GET /directory/<dir> → merge proxy list with in-memory filenames
				if (req.method === 'GET' && url.startsWith('/directory/')) {
					// FileBrowser appends trailing slashes — strip them for consistent key lookup
					const dirPath = (`/${url.slice('/directory/'.length)}`).replace(/\/$/, '') || '/';
					const inMem = Array.from(store.keys())
						.filter(p => {
							if (!p.startsWith(`${dirPath}/`)) return false;

							// direct children only
							return !p.slice(dirPath.length + 1).includes('/');
						})
						.map(p => p.split('/').pop() as string);

					if (inMem.length > 0) {
						(globalThis as any).fetch(`http://127.0.0.1:8000${url}`)
							.then((r: Response) => (r.ok ? r.json() : []))
							.then((base: unknown) => {
								const list = Array.isArray(base) ? (base as string[]) : [];
								const merged = [...new Set([...list, ...inMem])];
								res.writeHead(200, { 'Content-Type': 'application/json' });
								res.end(JSON.stringify(merged));
							})
							.catch(() => {
								res.writeHead(200, { 'Content-Type': 'application/json' });
								res.end(JSON.stringify(inMem));
							});
						return;
					}
				}

				next();
			});
		},
	};
}

// https://vitejs.dev/config/
export default defineConfig({
	plugins: [preact(), devMockWriteHandler()],
	define: {
		__APP_VERSION__: JSON.stringify('1.0.0'),
	},
	build: {
		outDir: '../components/https_server/www/',
		emptyOutDir: true,
		rollupOptions: {
			output: {
				entryFileNames: 'assets/app.js',
				chunkFileNames: 'assets/[name].js',
				assetFileNames: 'assets/[name].[ext]',
			},
		},
	},
	server: {
		proxy: {
			'/directory': 'http://127.0.0.1:8000',
			'/file': 'http://127.0.0.1:8000',
			'/config': 'http://127.0.0.1:8000',
		},
	},
});
