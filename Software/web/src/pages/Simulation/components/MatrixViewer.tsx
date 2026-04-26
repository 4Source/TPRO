import worldmapImage from '../../../assets/worldmap.svg';
import type { ThinPlateSpline } from '../../../tps';
import type { Coordinates } from '../types';
import { useZoomPan } from '../hooks/useZoomPan';
import { WIDTH, HEIGHT } from '../constants';

type Props = {
	ledColors: string[];
	showMap: boolean;
	mapOpacity: number;
	dotsOpacity: number;
	clickToCoordinatesMode: boolean;
	tps: ThinPlateSpline | null;

	// eslint-disable-next-line no-unused-vars
	onCoordinateClick: (_coords: Coordinates) => void;
};

export function MatrixViewer({ ledColors, showMap, mapOpacity, dotsOpacity, clickToCoordinatesMode, tps, onCoordinateClick }: Props) {
	const { zoom, pan, isDragging, handleWheel, handleMouseDown, handleMouseMove, handleMouseUp } = useZoomPan();

	const handleMatrixClick = (e: MouseEvent) => {
		if (!clickToCoordinatesMode || !tps || isDragging) return;

		const rect = (e.currentTarget as HTMLElement).getBoundingClientRect();
		const mmX = Math.max(0, Math.min(WIDTH, ((e.clientX - rect.left) / rect.width) * WIDTH));
		const mmY = Math.max(0, Math.min(HEIGHT, ((e.clientY - rect.top) / rect.height) * HEIGHT));
		const [lon, lat] = tps.transform(mmX, mmY);

		onCoordinateClick({ mmX, mmY, lon, lat });
	};

	return (
		<div
			class={`matrix-container ${zoom > 1 ? 'zoomed' : ''}`}
			onWheel={handleWheel}
			onMouseDown={handleMouseDown}
			onMouseMove={handleMouseMove}
			onMouseUp={handleMouseUp}
			onMouseLeave={handleMouseUp}
		>
			<div
				class="matrix-content"
				style={{ transform: `translate(${pan.x}px, ${pan.y}px) scale(${zoom})` }}
			>
				{showMap && (
					<div class="world-map-background" style={{ opacity: mapOpacity }}>
						<img src={worldmapImage} class="world-map-overlay" />
					</div>
				)}
				<div
					class={`matrix-grid ${clickToCoordinatesMode ? 'cursor-crosshair' : 'cursor-default'}`}
					onClick={handleMatrixClick}
				>
					{ledColors.map((color, index) => (
						<div
							key={index}
							class="dot"
							title={`LED ${index}`}
							style={{ opacity: dotsOpacity, backgroundColor: color }}
						/>
					))}
				</div>
			</div>
		</div>
	);
}
