import { useState, useEffect } from 'preact/hooks';
import './style.css';
import type { LedData, Coordinates } from './types';
import { COLS, ROWS, WIDTH, HEIGHT, LED_WIDTH, LED_HEIGHT, TOTAL_LEDS, PADDING_X, PADDING_Y } from './constants';
import { useWebSocket } from './hooks/useWebSocket';
import { useTps } from './hooks/useTps';
import { SettingsMenu } from './components/SettingsMenu';
import { MatrixViewer } from './components/MatrixViewer';
import { TimeControls } from './components/TimeControls';
import { QuickEffectPanel } from './components/QuickEffectPanel';
import { computeLedContinentMapping, CONTINENT_DEFAULTS } from '../../continent-mapping';

export function Simulation() {
	const ledColors = useWebSocket(TOTAL_LEDS);
	const { tps, sourcePoints, loadFromFile } = useTps();

	const [showMap, setShowMap] = useState(true);
	const [dotsOpacity, setDotsOpacity] = useState(0.5);
	const [mapOpacity, setMapOpacity] = useState(1);
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
	const [effectsPath, setEffectsPath] = useState('/effects');
	const [activeEffectPath, setActiveEffectPath] = useState('');
	const [dotSize, setDotSize] = useState(50);

	useEffect(() => {
		setGridData(null);
	}, [sourcePoints.length]);

	useEffect(() => {
		Promise.all([
			fetch('/config?effects_path').then(r => (r.ok ? r.text() : '/effects')).catch(() => '/effects'),
			fetch('/config?current_effect').then(r => (r.ok ? r.text() : '')).catch(() => ''),
		]).then(([ePath, activePath]) => {
			const clean = (s: string) => { const v = s.trim().replace(/^"|"$/g, ''); return v.startsWith('<') ? '' : v; };
			setEffectsPath(clean(ePath) || '/effects');
			const ap = clean(activePath);
			setActiveEffectPath(ap.endsWith('.json') ? ap : '');
		});
	}, []);

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
		setTimeout(async () => {
			const newData: LedData[] = [];
			for (let row = 0; row < ROWS; row++) {
				for (let col = 0; col < COLS; col++) {
					const index = (row * COLS) + col;
					const mmX = PADDING_X + ((col / (COLS - 1)) * LED_WIDTH);
					const mmY = PADDING_Y + ((row / (ROWS - 1)) * LED_HEIGHT);
					const [lon, lat] = tps.transform(mmX, mmY);
					const continents = Object.assign({}, CONTINENT_DEFAULTS);
					newData.push({ index, col, row, mmX, mmY, lon, lat, continents });
				}
			}

			const continents_mapping = await computeLedContinentMapping(newData.map(v => {
				return { index: v.index, x: v.mmX, y: v.mmY };
			}));
			for (let row = 0; row < ROWS; row++) {
				for (let col = 0; col < COLS; col++) {
					const index = (row * COLS) + col;
					newData[index].continents = Object.assign({}, CONTINENT_DEFAULTS, continents_mapping[index]);
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
				continents: d.continents,
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
					<div class="sim-toolbar-left">
						<button class="sim-toolbar-btn" onClick={() => setMenuOpen(true)}>
							Einstellungen
						</button>
						<QuickEffectPanel
							effectsPath={effectsPath}
							onEffectChanged={setActiveEffectPath}
						/>
					</div>
					<span class="sim-active-effect">
						{(() => {
							if (!activeEffectPath) return 'Aktueller Effekt: Keiner';
							const t = activeEffectPath.split('/').pop()?.replace('.json', '') ?? '';
							const label = t.startsWith('my_timeline_') ? t.slice('my_timeline_'.length) :
								t.startsWith('my_') ? t.slice('my_'.length) :
									t;
							return `Aktueller Effekt: ${label}`;
						})()}
					</span>
				</div>
				<MatrixViewer
					ledColors={ledColors}
					showMap={showMap}
					mapOpacity={mapOpacity}
					dotsOpacity={dotsOpacity}
					dotSize={dotSize}
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
				onEffectActivated={setActiveEffectPath}
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
				dotSize={dotSize}
				onToggleMap={() => setShowMap(v => !v)}
				onMapOpacityChange={setMapOpacity}
				onDotsOpacityChange={setDotsOpacity}
				onDotSizeChange={setDotSize}
			/>
		</div>
	);
}
