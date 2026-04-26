import { useState, useEffect } from 'preact/hooks';

export function useWebSocket(totalLeds: number): string[] {
	const [ledColors, setLedColors] = useState<string[]>(new Array(totalLeds).fill('rgb(0, 0, 0)'));

	useEffect(() => {
		const ip = window.location.hostname;
		const ws = new WebSocket(`wss://${ip}/ws`);
		ws.binaryType = 'arraybuffer';

		ws.onopen = () => console.log('WebSocket verbunden!');
		ws.onerror = (error) => console.error('WebSocket Fehler:', error);
		ws.onclose = () => console.log('WebSocket getrennt.');

		ws.onmessage = (event) => {
			if (!(event.data instanceof ArrayBuffer)) return;

			const view = new Uint8Array(event.data);
			const newColors = new Array(totalLeds);
			let i = 0;
			for (let led = 0; led < totalLeds; led++) {
				newColors[led] = `rgb(${view[i++]}, ${view[i++]}, ${view[i++]})`;
			}
			setLedColors(newColors);
		};

		return () => {
			if (ws.readyState === WebSocket.OPEN) ws.close();
		};
	}, [totalLeds]);

	return ledColors;
}
