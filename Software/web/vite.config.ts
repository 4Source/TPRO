import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';

// https://vitejs.dev/config/
export default defineConfig({
	plugins: [preact()],
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
		},
	},
});
