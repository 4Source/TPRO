import { useState, useMemo } from 'preact/hooks';
import './style.css';
import { ThinPlateSpline } from '../../tps';
import worldmapImage from '../../assets/worldmap.svg';

// Typ-Definition für unsere berechneten LEDs
type LedData = {
	index: number;
	col: number;
	row: number;
	mmX: number;
	mmY: number;
	lon: number;
	lat: number;
};

export function Simulation() {
	// TODO: Grid mit festen Abständen und Padding zur Außenseite padding x&y

	/** LED Spaltenanzahl */
	const COLS = 60;

	/** LED Zeilenanzahl */
	const ROWS = 54;

	/** Breite der LED-Karte in mm */
	const WIDTH = 3000;

	/** Höhe der LED-Karte in mm */
	const HEIGHT = 1800;

	/** Padding an den Seiten in mm  */
	const PADDING = 1;

	// Verfügbarer Platz für das Grid
	const GRID_WIDTH = WIDTH - (2 * PADDING);
	const GRID_HEIGHT = HEIGHT - (2 * PADDING);

	const totalDots = COLS * ROWS;

	// --- STATES ---
	const [matrixState /* setMatrixState*/] = useState(new Array(totalDots).fill(false));
	const [showMap, setShowMap] = useState(true);
	const [dotsOpacity, setDotsOpacity] = useState(1);
	const [mapOpacity, setMapOpacity] = useState(0.5);
	const [timeValue, setTimeValue] = useState(632);
	const [isPlaying, setIsPlaying] = useState(false);
	const [speed, setSpeed] = useState('x1');
	const [selectedDate, setSelectedDate] = useState(new Date().toISOString().split('T')[0]);
	const [isCalculating, setIsCalculating] = useState(false);
	const [gridData, setGridData] = useState<LedData[] | null>(null);

	// const [hoveredLed, setHoveredLed] = useState<LedData | null>(null);
	const [sourcePoints, setSourcePoints] = useState<number[][]>([]);
	const [targetPoints, setTargetPoints] = useState<number[][]>([]);
	const [clickToCoordinatesMode, setClickToCoordinatesMode] = useState(false);
	const [lastClickedCoordinates, setLastClickedCoordinates] = useState<{ mmX: number, mmY: number, lon: number, lat: number } | null>(null);
	const [ledNumberInput, setLedNumberInput] = useState('');
	const [ledNumberResult, setLedNumberResult] = useState<LedData | null>(null);
	const [zoom, setZoom] = useState(1);
	const [pan, setPan] = useState({ x: 0, y: 0 });
	const [isDragging, setIsDragging] = useState(false);
	const [dragStart, setDragStart] = useState({ x: 0, y: 0 });

	/** TPS-INSTANZ */
	const tps = useMemo(() => {
		if (sourcePoints.length >= 3 && targetPoints.length >= 3) {
			return new ThinPlateSpline(sourcePoints, targetPoints);
		}
		return null;
	}, [sourcePoints, targetPoints]);

	// --- HILFSFUNKTIONEN ---
	const formatTime = (minutes: number) => {
		const hrs = Math.floor(minutes / 60);
		const mins = minutes % 60;
		return `${hrs.toString().padStart(2, '0')}:${mins.toString().padStart(2, '0')}`;
	};

	/** CLICK TO COORDINATES MODE  */
	const handleMatrixClick = (e: MouseEvent) => {
		if (!clickToCoordinatesMode || !tps || isDragging) return;

		// Das Element, auf das wir geklickt haben (.matrix-grid)
		const grid = e.currentTarget as HTMLElement;

		// Liefert die exakte Box des Gitters auf dem Bildschirm (INKL. Zoom & Pan!)
		const rect = grid.getBoundingClientRect();

		// Wie viele Pixel ist die Maus vom linken/oberen Rand des Gitters entfernt?
		const clickX = e.clientX - rect.left;
		const clickY = e.clientY - rect.top;

		// An welcher Position im Gitter befinden wir uns in Prozent? (0.0 bis 1.0)
		// Da rect.width beim Zoomen wächst, stimmt dieser Prozentwert IMMER.
		const percentX = clickX / rect.width;
		const percentY = clickY / rect.height;

		// Prozent auf die echten mm umlegen (WIDTH = 3000, HEIGHT = 1800)
		const mmX = percentX * WIDTH;
		const mmY = percentY * HEIGHT;

		// Clamp zur Sicherheit
		const clampedX = Math.max(0, Math.min(WIDTH, mmX));
		const clampedY = Math.max(0, Math.min(HEIGHT, mmY));

		// Transformiere zu Weltkoordinaten
		const [lon, lat] = tps.transform(clampedX, clampedY);

		const coords = { mmX: clampedX, mmY: clampedY, lon, lat };
		setLastClickedCoordinates(coords);
		console.log('Clicked coordinates:', coords);
	};

	/** ZOOM & PAN */
	const handleWheel = (e: WheelEvent) => {
		e.preventDefault();

		// Das ist der matrix-container
		const wrapper = e.currentTarget as HTMLElement;
		const wrapperRect = wrapper.getBoundingClientRect();

		const zoomSpeed = 0.1;
		const newZoom = Math.max(1, zoom - (e.deltaY > 0 ? zoomSpeed : -zoomSpeed));

		// Auch hier: 20px abziehen, damit der Zoom exakt auf die Maus zentriert bleibt
		const rawX = e.clientX - wrapperRect.left - 20;
		const rawY = e.clientY - wrapperRect.top - 20;

		// Logische Position vor dem neuen Zoom
		const logicalMouseX = (rawX - pan.x) / zoom;
		const logicalMouseY = (rawY - pan.y) / zoom;

		const zoomDiff = newZoom - zoom;

		setPan({
			x: pan.x - (logicalMouseX * zoomDiff),
			y: pan.y - (logicalMouseY * zoomDiff),
		});

		setZoom(newZoom);
	};

	const handleMouseDown = (e: MouseEvent) => {
		setIsDragging(true);
		setDragStart({ x: e.clientX - pan.x, y: e.clientY - pan.y });
	};

	const handleMouseMove = (e: MouseEvent) => {
		if (!isDragging) return;
		setPan({
			x: e.clientX - dragStart.x,
			y: e.clientY - dragStart.y,
		});
	};

	const handleMouseUp = () => {
		setIsDragging(false);
	};

	/** LED NUMBER LOOKUP  */
	const handleLedNumberSearch = () => {
		const ledNum = parseInt(ledNumberInput, 10);
		if (isNaN(ledNum) || !gridData) {
			setLedNumberResult(null);
			return;
		}

		const led = gridData.find(d => d.index === ledNum);
		setLedNumberResult(led || null);
	};

	// QGIS .points PARSER ---
	/*
        mapX,mapY,pixelX,pixelY,enable
        -80.74487929,25.29191503,687.78765690,755.72698745,1
    */
	const handleFileUpload = (e: Event) => {
		const file = (e.target as HTMLInputElement).files?.[0];
		if (!file) return;

		const reader = new FileReader();
		reader.onload = (event) => {
			const text = event.target?.result as string;

			/** Datei in Zeilen aufteilen */
			const lines = text.split('\n');

			const newSources: number[][] = [];
			const newTargets: number[][] = [];

			lines.forEach(line => {
				/** Leerzeichen entfernen und durch Komma splitten*/
				const parts = line.split(',').map(s => s.trim());

				// QGIS Format hat 5 Spalten: mapX, mapY, pixelX, pixelY, enable
				if (parts.length >= 5) {
					/** Longitude (Ziel) */
					const mapX = parseFloat(parts[0]);

					/** Latitude (Ziel) */
					const mapY = parseFloat(parts[1]);

					/** X in mm/Pixel (Quelle) */
					const pixelX = parseFloat(parts[2]);

					/** Y in mm/Pixel (Quelle) */
					const pixelY = parseFloat(parts[3]);

					/** 1 = aktiv, 0 = inaktiv, oder NaN bei Header */
					const enable = parseInt(parts[4], 10);

					// Überspringe Header-Zeile (wo Text statt Zahlen steht) und inaktive Punkte
					if (enable === 1 && !isNaN(mapX) && !isNaN(pixelX)) {
						newSources.push([pixelX, pixelY]);
						newTargets.push([mapX, mapY]);
					}
				}
			});

			if (newSources.length >= 3) {
				setSourcePoints(newSources);
				setTargetPoints(newTargets);

				// Grid resetten, da wir neue Ankerpunkte haben
				setGridData(null);
			}
			else {
				alert('Fehler: Zu wenige gültige, aktive Punkte in der Datei gefunden. Du brauchst mindestens 3.');
			}
		};

		// Datei als Text einlesen
		reader.readAsText(file);
	};

	/** BERECHNUNG DES GESAMTEN RASTERS*/
	const handleCalculateGrid = () => {
		if (!tps) {
			alert('Fehler: TPS nicht initialisiert. Bitte zuerst eine Datei hochladen mit mindestens 3 Ankerpunkten.');
			return;
		}
		setIsCalculating(true);

		setTimeout(() => {
			const newData: LedData[] = [];
			for (let row = 0; row < ROWS; row++) {
				for (let col = 0; col < COLS; col++) {
					// Berechne Position mit Padding
					const mmX = PADDING + (col * (GRID_WIDTH / (COLS - 1)));
					const mmY = PADDING + (row * (GRID_HEIGHT / (ROWS - 1)));

					const [lon, lat] = tps.transform(mmX, mmY);

					newData.push({
						index: (row * COLS) + col,
						col, row, mmX, mmY, lon, lat,
					});
				}
			}
			setGridData(newData);
			setIsCalculating(false);
		}, 100);
	};

	/** EXPORT */
	const handleExport = () => {
		if (!gridData) {
			alert('Fehler: Keine Gitterdaten vorhanden. Bitte zuerst Raster berechnen.');
			return;
		}

		const config = {
			version: 1,
			meta: {
				created: new Date().toISOString(),
			},
			grid: {
				cols: COLS,
				rows: ROWS,
				width: WIDTH,
				height: HEIGHT,
			},
			leds: gridData.map(d => ({
				index: d.index,
				x: d.col,
				y: d.row,
				mmX: parseFloat(d.mmX.toFixed(2)),
				mmY: parseFloat(d.mmY.toFixed(2)),
				lon: parseFloat(d.lon.toFixed(5)),
				lat: parseFloat(d.lat.toFixed(5)),
			})),
		};

		const blob = new Blob(
			[JSON.stringify(config, null, 2)],
			{ type: 'application/json' },
		);

		const url = URL.createObjectURL(blob);

		const a = document.createElement('a');
		a.href = url;
		a.download = 'led-config.json';
		a.click();

		URL.revokeObjectURL(url);
	};

	return (
		<div className="app-container">
			{/* File Upload & Grid Calculation Controls */}
			<div className="flex items-center gap-3 bg-gray-800 px-6 py-4 rounded-xl border border-gray-700 shadow-lg mb-4">
				<label className="text-sm font-semibold text-white cursor-pointer">
					📤 Punkte laden (.csv):
					<input
						type="file"
						accept=".csv,.txt"
						onChange={handleFileUpload}
						className="hidden"
					/>
				</label>
				<button
					onClick={handleCalculateGrid}
					disabled={!tps || isCalculating}
					className="px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 disabled:bg-gray-600 disabled:cursor-not-allowed font-semibold"
				>
					{isCalculating ? 'Berechne...' : 'Raster berechnen'}
				</button>
				<button
					onClick={handleExport}
					disabled={!gridData}
					className="px-4 py-2 bg-green-600 text-white rounded-lg hover:bg-green-700 disabled:bg-gray-600 disabled:cursor-not-allowed font-semibold"
				>
					💾 Exportieren
				</button>
				{sourcePoints.length > 0 && (
					<span className="text-xs text-gray-400">({sourcePoints.length} Ankerpunkte, {gridData?.length || 0} LEDs)</span>
				)}
			</div>

			{/* Click to Coordinates Mode Toggle */}
			<div className="flex items-center gap-3 bg-gray-800 px-6 py-4 rounded-xl border border-gray-700 shadow-lg mb-4">
				<div className="flex items-center gap-2">
					<input
						type="checkbox"
						id="clickMode"
						checked={clickToCoordinatesMode}
						onChange={() => setClickToCoordinatesMode(!clickToCoordinatesMode)}
						disabled={!tps}
						className="w-5 h-5 cursor-pointer accent-purple-500 disabled:cursor-not-allowed disabled:opacity-50"
					/>
					<label htmlFor="clickMode" className="text-sm font-semibold text-white cursor-pointer select-none">
						🎯 Click to Coordinates Mode
					</label>
				</div>
				{lastClickedCoordinates && (
					<div className="ml-auto text-xs text-gray-300 font-mono bg-gray-900 px-3 py-1.5 rounded">
						<div>mm: {lastClickedCoordinates.mmX.toFixed(1)}, {lastClickedCoordinates.mmY.toFixed(1)}</div>
						<div>lon/lat: {lastClickedCoordinates.lon.toFixed(5)}, {lastClickedCoordinates.lat.toFixed(5)}</div>
					</div>
				)}
			</div>

			{/* LED Number Lookup */}
			<div className="flex items-center gap-3 bg-gray-800 px-6 py-4 rounded-xl border border-gray-700 shadow-lg mb-4">
				<label className="text-sm font-semibold text-white">🔍 LED Nummer:</label>
				<input
					type="number"
					value={ledNumberInput}
					onChange={(e) => setLedNumberInput((e.target as HTMLInputElement).value)}
					onKeyPress={(e) => {
						if ((e as KeyboardEvent).key === 'Enter') handleLedNumberSearch();
					}}
					placeholder="LED Index"
					disabled={!gridData}
					className="bg-gray-700 text-white text-sm rounded-lg px-3 py-2 outline-none border border-gray-600 focus:border-blue-500 w-32 disabled:opacity-50"
				/>
				<button
					onClick={handleLedNumberSearch}
					disabled={!gridData}
					className="px-4 py-2 bg-cyan-600 text-white rounded-lg hover:bg-cyan-700 disabled:bg-gray-600 disabled:cursor-not-allowed font-semibold text-sm"
				>
					Suchen
				</button>
				{ledNumberResult && (
					<div className="ml-auto text-xs text-gray-300 font-mono bg-gray-900 px-3 py-1.5 rounded">
						<div>LED #{ledNumberResult.index} (Col: {ledNumberResult.col}, Row: {ledNumberResult.row})</div>
						<div>mm: {ledNumberResult.mmX.toFixed(1)}, {ledNumberResult.mmY.toFixed(1)}</div>
						<div>lon/lat: {ledNumberResult.lon.toFixed(5)}, {ledNumberResult.lat.toFixed(5)}</div>
					</div>
				)}
			</div>

			{/* Header / Controls */}
			<div className="flex items-center justify-between bg-gray-800 px-6 py-4 rounded-xl border border-gray-700 shadow-lg">
				<div className="flex items-center gap-3">
					<input
						type="checkbox"
						id="worldMap"
						checked={showMap}
						onChange={() => setShowMap(!showMap)}
						className="w-6 h-6 cursor-pointer accent-blue-500"
					/>
					<label htmlFor="worldMap" className="text-base font-semibold cursor-pointer select-none text-white">
						Show World Map Image
					</label>
				</div>

				<div className="flex items-center gap-3 bg-gray-900 px-4 py-1.5 rounded-lg border border-gray-700">
					<span className="text-xs text-gray-400 uppercase tracking-wider font-bold">Map Opacity</span>
					<input
						type="range"
						min="0"
						max="1"
						step="0.01"
						value={mapOpacity}
						onInput={(e) => setMapOpacity(parseFloat((e.target as HTMLInputElement).value))}
						className="w-32 h-1.5 accent-blue-500 cursor-pointer"
						disabled={!showMap}
					/>
					<span className="text-xs font-mono w-8 text-right text-white">{Math.round(mapOpacity * 100)}%</span>
				</div>

				<div className="flex items-center gap-3 bg-gray-900 px-4 py-1.5 rounded-lg border border-gray-700">
					<span className="text-xs text-gray-400 uppercase tracking-wider font-bold">Points Opacity</span>
					<input
						type="range"
						min="0"
						max="1"
						step="0.01"
						value={dotsOpacity}
						onInput={(e) => setDotsOpacity(parseFloat((e.target as HTMLInputElement).value))}
						className="w-32 h-1.5 accent-purple-500 cursor-pointer"
					/>
					<span className="text-xs font-mono w-8 text-right text-white">{Math.round(dotsOpacity * 100)}%</span>
				</div>
			</div>

			{/* Matrix Bereich */}
			<div
				className={`matrix-container ${zoom > 1 ? 'zoomed' : ''}`}
				onWheel={handleWheel}
				onMouseDown={handleMouseDown}
				onMouseMove={handleMouseMove}
				onMouseUp={handleMouseUp}
				onMouseLeave={handleMouseUp}
			>
				<div
					className="matrix-content"
					style={{ transform: `translate(${pan.x}px, ${pan.y}px) scale(${zoom})` }}
				>
					{showMap && (
						<img
							src={worldmapImage}
							className="world-map-overlay"
							style={{ opacity: mapOpacity }}
						/>
					)}
					<div
						className={`matrix-grid ${clickToCoordinatesMode ? 'cursor-crosshair' : 'cursor-default'}`}
						onClick={handleMatrixClick}
					>
						{matrixState.map((active, index) => (
							<div
								key={index}
								className={`dot ${active ? 'active' : 'inactive'}`}
								title={`LED ${index}`}
								style={{ opacity: dotsOpacity }}
							/>
						))}
					</div>
				</div>
			</div>

			{/* Bottom Controls / Time Slider */}
			<div className="flex items-center gap-4 bg-gray-800 p-4 rounded-xl border border-gray-700 shadow-lg w-full">
				<div className="flex items-center gap-2 min-w-[160px]">
					<input
						type="date"
						value={selectedDate}
						onChange={(e) => setSelectedDate((e.target as HTMLInputElement).value)}
						className="bg-gray-700 text-white text-sm rounded-lg px-3 py-2 outline-none border border-gray-600 focus:border-blue-500 w-full"
					/>
				</div>

				<div className="flex flex-grow items-center gap-4 px-2">
					<span className="text-lg font-mono text-blue-400 min-w-[55px] text-center">{formatTime(timeValue)}</span>
					<div className="slider-container">
						<div className="slider-horizontal-line" />
						<input
							type="range"
							min="0"
							max="1439"
							value={timeValue}
							onInput={(e) => setTimeValue(parseInt((e.target as HTMLInputElement).value, 10))}
						/>
					</div>
				</div>

				<div className="flex items-center gap-2">
					<div className="flex bg-gray-900 rounded-lg p-1 border border-gray-700">
						<button
							onClick={() => setIsPlaying(true)}
							className={`px-4 py-1.5 rounded-md text-xs font-bold transition-all ${isPlaying ? 'bg-slate-200 text-black shadow-inner' : 'text-gray-400 hover:text-white'}`}
						>
							START
						</button>
						<button
							onClick={() => setIsPlaying(false)}
							className={`px-4 py-1.5 rounded-md text-xs font-bold transition-all ${!isPlaying ? 'bg-slate-200 text-black shadow-inner' : 'text-gray-400 hover:text-white'}`}
						>
							STOP
						</button>
					</div>

					<select
						value={speed}
						onChange={(e) => setSpeed((e.target as HTMLSelectElement).value)}
						className="bg-gray-700 text-white text-sm rounded-lg px-3 py-2 outline-none border border-gray-600 focus:border-blue-500 min-w-[80px]"
					>
						{['x1', 'x2', 'x10', 'x24', 'x240', 'x360', 'x3600'].map(s => (
							<option key={s} value={s}>{s}</option>
						))}
					</select>
				</div>
			</div>
		</div>
	);
}
