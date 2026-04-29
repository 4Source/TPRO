import { useState, useEffect } from 'preact/hooks';
import './style.css';
import type { LedData, Coordinates } from './types';
import { COLS, ROWS, WIDTH, HEIGHT, LED_WIDTH, LED_HEIGHT, TOTAL_LEDS, PADDING_X, PADDING_Y } from './constants';
import { useWebSocket } from './hooks/useWebSocket';
import { useTps } from './hooks/useTps';
import { SettingsMenu } from './components/SettingsMenu';
import { MatrixViewer } from './components/MatrixViewer';
import { TimeControls } from './components/TimeControls';

export function Simulation() {
	const ledColors = useWebSocket(TOTAL_LEDS);
	const { tps, sourcePoints, loadFromFile } = useTps();

	const [showMap, setShowMap] = useState(true);
	const [dotsOpacity, setDotsOpacity] = useState(1);
	const [mapOpacity, setMapOpacity] = useState(0.5);
	const [timeValue, setTimeValue] = useState(632);
	const [isPlaying, setIsPlaying] = useState(false);
	const [speed, setSpeed] = useState('x1');
	const [selectedDate, setSelectedDate] = useState(new Date().toISOString().split('T')[0]);
	const [isCalculating, setIsCalculating] = useState(false);
	const [gridData, setGridData] = useState<LedData[] | null>(null);
	const [clickToCoordinatesMode, setClickToCoordinatesMode] = useState(false);
	const [lastCoordinates, setLastCoordinates] = useState<Coordinates | null>(null);
	const [ledNumberInput, setLedNumberInput] = useState('');
	const [ledNumberResult, setLedNumberResult] = useState<LedData | null>(null);
	const [menuOpen, setMenuOpen] = useState(false);
	const [darkMode, setDarkMode] = useState(true);

	useEffect(() => {
		setGridData(null);
	}, [sourcePoints.length]);

	useEffect(() => {
		document.documentElement.classList.toggle('page-light-mode', !darkMode);
		document.documentElement.classList.toggle('page-dark-mode', darkMode);
		return () => {
			document.documentElement.classList.remove('page-light-mode', 'page-dark-mode');
		};
	}, [darkMode]);

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
					const mmX = PADDING_X + ((col / (COLS - 1)) * LED_WIDTH);
					const mmY = PADDING_Y + ((row / (ROWS - 1)) * LED_HEIGHT);
					const [lon, lat] = tps.transform(mmX, mmY);
					newData.push({ index: (row * COLS) + col, col, row, mmX, mmY, lon, lat });
				}
			}
			setGridData(newData);
			setIsCalculating(false);
		}, 100);
	};

	const handleExport = () => {
		if (!gridData) {
			alert('Fehler: Keine Gitterdaten vorhanden. Bitte zuerst Raster berechnen.');
			return;
		}
		const config = {
			version: 1,
			meta: { created: new Date().toISOString() },
			grid: { cols: COLS, rows: ROWS, width: WIDTH, height: HEIGHT },
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
		const blob = new Blob([JSON.stringify(config, null, 2)], { type: 'application/json' });
		const url = URL.createObjectURL(blob);
		const a = document.createElement('a');
		a.href = url;
		a.download = 'led-config.json';
		a.click();
		URL.revokeObjectURL(url);
	};

	const handleLedSearch = () => {
		const ledNum = parseInt(ledNumberInput, 10);
		setLedNumberResult((!isNaN(ledNum) && gridData) ?
			gridData.find(d => d.index === ledNum) ?? null :
			null,
		);
	};

	return (
		<div class={`simulation-page${darkMode ? '' : ' sim-light'}`}>
			<div class="app-container">
				<div class="sim-toolbar">
					<button class="sim-toolbar-btn" onClick={() => setMenuOpen(true)}>
						Einstellungen
					</button>
				</div>
				<MatrixViewer
					ledColors={ledColors}
					showMap={showMap}
					mapOpacity={mapOpacity}
					dotsOpacity={dotsOpacity}
					clickToCoordinatesMode={clickToCoordinatesMode}
					tps={tps}
					onCoordinateClick={setLastCoordinates}
				/>
				<TimeControls
					selectedDate={selectedDate}
					timeValue={timeValue}
					isPlaying={isPlaying}
					speed={speed}
					onDateChange={setSelectedDate}
					onTimeChange={setTimeValue}
					onPlayingChange={setIsPlaying}
					onSpeedChange={setSpeed}
				/>
			</div>
			<SettingsMenu
				open={menuOpen}
				onClose={() => setMenuOpen(false)}
				darkMode={darkMode}
				onToggleDarkMode={() => setDarkMode(v => !v)}
				tps={tps}
				isCalculating={isCalculating}
				gridData={gridData}
				sourcePoints={sourcePoints}
				onFileUpload={loadFromFile}
				onCalculate={handleCalculateGrid}
				onExport={handleExport}
				clickToCoordinatesMode={clickToCoordinatesMode}
				lastCoordinates={lastCoordinates}
				onToggleCoordinates={() => setClickToCoordinatesMode(v => !v)}
				ledNumberInput={ledNumberInput}
				ledNumberResult={ledNumberResult}
				onLedInputChange={setLedNumberInput}
				onLedSearch={handleLedSearch}
				showMap={showMap}
				mapOpacity={mapOpacity}
				dotsOpacity={dotsOpacity}
				onToggleMap={() => setShowMap(v => !v)}
				onMapOpacityChange={setMapOpacity}
				onDotsOpacityChange={setDotsOpacity}
			/>
		</div>
	);
}
