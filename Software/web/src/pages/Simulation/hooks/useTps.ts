import { useState, useMemo } from 'preact/hooks';
import { ThinPlateSpline } from '../../../tps';

export function useTps() {
	const [sourcePoints, setSourcePoints] = useState<number[][]>([]);
	const [targetPoints, setTargetPoints] = useState<number[][]>([]);

	const tps = useMemo(() => {
		if (sourcePoints.length >= 3 && targetPoints.length >= 3) {
			return new ThinPlateSpline(sourcePoints, targetPoints);
		}
		return null;
	}, [sourcePoints, targetPoints]);

	/*
	 * QGIS .points file format:
	 * mapX,mapY,pixelX,pixelY,enable
	 * -80.74487929,25.29191503,687.78765690,755.72698745,1
	 */
	const loadFromFile = (e: Event, onLoad?: () => void) => {
		const file = (e.target as HTMLInputElement).files?.[0];
		if (!file) return;

		const reader = new FileReader();
		reader.onload = (event) => {
			const text = event.target?.result as string;
			const newSources: number[][] = [];
			const newTargets: number[][] = [];

			text.split('\n').forEach(line => {
				const parts = line.split(',').map(s => s.trim());
				if (parts.length < 5) return;

				const mapX = parseFloat(parts[0]);
				const mapY = parseFloat(parts[1]);
				const pixelX = parseFloat(parts[2]);
				const pixelY = parseFloat(parts[3]);
				const enable = parseInt(parts[4], 10);

				if (enable === 1 && !isNaN(mapX) && !isNaN(pixelX)) {
					newSources.push([pixelX, pixelY]);
					newTargets.push([mapX, mapY]);
				}
			});

			if (newSources.length >= 3) {
				setSourcePoints(newSources);
				setTargetPoints(newTargets);
				onLoad?.();
			}
			else {
				alert('Fehler: Zu wenige gültige, aktive Punkte in der Datei gefunden. Du brauchst mindestens 3.');
			}
		};
		reader.readAsText(file);
	};

	return { tps, sourcePoints, loadFromFile };
}
