import { useState, useEffect } from 'preact/hooks';
import { OpacitySlider } from '../../../components/OpacitySlider/OpacitySlider';
import { EffectPanel } from './EffectPanel';
import type { ThinPlateSpline } from '../../../tps';
import type { LedData, Coordinates } from '../types';
import { startSunDataGeneration } from '../../../sunapi';

type Props = {
	open: boolean;
	onClose: () => void;
	darkMode: boolean;
	onToggleDarkMode: () => void;
	tps: ThinPlateSpline | null;
	isCalculating: boolean;
	gridData: LedData[] | null;
	sourcePoints: number[][];

	onEffectActivated: (_path: string) => void;

	onFileUpload: (_e: Event) => void;
	onCalculate: () => void;
	onExport: () => void;
	clickToCoordinatesMode: boolean;
	lastCoordinates: Coordinates | null;
	onToggleCoordinates: () => void;
	ledNumberInput: string;
	ledNumberResult: LedData | null;

	onLedInputChange: (_value: string) => void;
	onLedSearch: () => void;
	showMap: boolean;
	mapOpacity: number;
	dotsOpacity: number;
	dotSize: number;
	onToggleMap: () => void;

	onMapOpacityChange: (_value: number) => void;

	onDotsOpacityChange: (_value: number) => void;

	onDotSizeChange: (_value: number) => void;
};

export function SettingsMenu({
	open, onClose,
	darkMode, onToggleDarkMode,
	tps, isCalculating, gridData, sourcePoints, onFileUpload, onCalculate, onExport,
	clickToCoordinatesMode, lastCoordinates, onToggleCoordinates,
	ledNumberInput, ledNumberResult, onLedInputChange, onLedSearch,
	showMap, mapOpacity, dotsOpacity, dotSize, onToggleMap, onMapOpacityChange, onDotsOpacityChange, onDotSizeChange,
	onEffectActivated,
}: Props) {
	const [showEffects, setShowEffects] = useState(false);
	const [activeFrom, setActiveFrom] = useState('07:00');
	const [activeTo, setActiveTo] = useState('18:00');

	useEffect(() => {
		const clean = (t: string) => t.trim().replace(/^"|"$/g, '');
		Promise.all([
			fetch('/config?active_from').then(r => (r.ok ? r.text() : '')).catch(() => ''),
			fetch('/config?active_to').then(r => (r.ok ? r.text() : '')).catch(() => ''),
		]).then(([from, to]) => {
			const f = clean(from);
			const t = clean(to);
			if (/^\d{2}:\d{2}$/.test(f)) setActiveFrom(f);
			if (/^\d{2}:\d{2}$/.test(t)) setActiveTo(t);
		});
	}, []);

	const [sunStatus, setSunStatus] = useState<string | null>(null);
	const [sunProgress, setSunProgress] = useState('');

	const handleGenerateSunData = () => {
		if (!gridData) return;

		// Mapping deiner LedData auf das Format der API-Funktion
		const leds = gridData.map(led => ({
			index: led.index,
			x: led.col,
			y: led.row,
			lat: led.lat,
			lon: led.lon,
		}));

		setSunStatus('Starte...');
		startSunDataGeneration(leds, 2024, (status, progress) => {
			setSunStatus(status);
			setSunProgress(progress);
		});
	};
	const saveTime = (key: 'active_from' | 'active_to', value: string) => {
		if (/^\d{2}:\d{2}$/.test(value)) {
			fetch(`/config?${key}=${encodeURIComponent(value)}`, { method: 'PUT' }).catch(() => { });
		}
	};

	const handleClose = () => {
		setShowEffects(false);
		onClose();
	};

	return (
		<>
			<div class={`settings-backdrop${open ? ' open' : ''}`} onClick={handleClose} />
			<div class={`settings-panel${open ? ' open' : ''}`}>

				{/* Header */}
				<div class="settings-panel-header">
					{showEffects ? (
						<button class="settings-back-btn" onClick={() => setShowEffects(false)}>
							← Einstellungen
						</button>
					) : (
						<span>Einstellungen</span>
					)}
					<button class="settings-close-btn" onClick={handleClose}>✕</button>
				</div>

				{/* Body */}
				{showEffects ? (
					<div class="settings-panel-body">
						<EffectPanel onEffectActivated={onEffectActivated} />
					</div>
				) : (
					<div class="settings-panel-body">

						{/* Effekt */}
						<div class="settings-section">
							<p class="settings-section-title">Effekt</p>
							<button class="settings-nav-btn" onClick={() => setShowEffects(true)}>
								Effekt auswählen
								<span class="settings-nav-arrow">→</span>
							</button>
						</div>

						{/* Zeitschaltung */}
						<div class="settings-section">
							<p class="settings-section-title">Zeitschaltung</p>
							<div class="settings-time-row">
								<label class="settings-time-label">Von</label>
								<input
									type="time"
									value={activeFrom}
									onInput={(e) => setActiveFrom((e.target as HTMLInputElement).value)}
									onChange={(e) => saveTime('active_from', (e.target as HTMLInputElement).value)}
									class="settings-time-input"
								/>
								<label class="settings-time-label">Bis</label>
								<input
									type="time"
									value={activeTo}
									onInput={(e) => setActiveTo((e.target as HTMLInputElement).value)}
									onChange={(e) => saveTime('active_to', (e.target as HTMLInputElement).value)}
									class="settings-time-input"
								/>
							</div>
						</div>

						{/* Sichtbarkeit */}
						<div class="settings-section">
							<p class="settings-section-title">Sichtbarkeit</p>
							<label class="flex items-center gap-2 cursor-pointer select-none mb-3">
								<input
									type="checkbox"
									checked={showMap}
									onChange={onToggleMap}
									class="w-5 h-5 cursor-pointer accent-blue-500"
								/>
								<span class="settings-label-text">Weltkarte anzeigen</span>
							</label>
							<div class="flex flex-col gap-2">
								<OpacitySlider label="Karte" value={mapOpacity} accent="blue" disabled={!showMap} onChange={onMapOpacityChange} />
								<OpacitySlider label="Punkte" value={dotsOpacity} accent="purple" onChange={onDotsOpacityChange} />
							</div>
							<div class="flex items-center gap-3 bg-gray-900 px-4 py-1.5 rounded-lg border border-gray-700 mt-3">
								<span class="text-xs text-gray-400 uppercase tracking-wider font-bold">Grösse</span>
								<input
									type="range"
									min="0"
									max="100"
									step="1"
									value={dotSize}
									onInput={(e) => onDotSizeChange(parseInt((e.target as HTMLInputElement).value, 10))}
									class="w-32 h-1.5 cursor-pointer accent-green-500"
								/>
								<span class="text-xs font-mono w-8 text-right text-white">{dotSize}%</span>
							</div>
						</div>

						{/* Datei */}
						<div class="settings-section">
							<p class="settings-section-title">Datei</p>
							<label class="settings-file-label">
								Punkte laden (.csv / .txt)
								<input type="file" accept=".csv,.txt" onChange={onFileUpload} class="hidden" />
							</label>
							{sourcePoints.length > 0 && (
								<p class="settings-hint">{sourcePoints.length} Ankerpunkte · {gridData?.length ?? 0} LEDs</p>
							)}
							<div class="flex gap-2 mt-2">
								<button
									onClick={onCalculate}
									disabled={!tps || isCalculating}
									class="flex-1 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 disabled:bg-gray-600 disabled:cursor-not-allowed text-sm font-semibold"
								>
									{isCalculating ? 'Berechne...' : 'Raster berechnen'}
								</button>
								<button
									onClick={onExport}
									disabled={!gridData}
									class="flex-1 py-2 bg-green-600 text-white rounded-lg hover:bg-green-700 disabled:bg-gray-600 disabled:cursor-not-allowed text-sm font-semibold"
								>
									Exportieren
								</button>
							</div>

							<div class="flex gap-2 mt-2">
								<button
									onClick={handleGenerateSunData}
									disabled={(!gridData || sunStatus !== null) && sunStatus !== 'Abgeschlossen'}
									class="w-full py-2 bg-amber-500 text-white rounded-lg hover:bg-amber-600 disabled:bg-gray-600 disabled:cursor-not-allowed text-sm font-semibold"
								>
									{sunStatus ? 'Generiere Jahr...' : 'Sonnendaten (Jahr) auf SD'}
								</button>

								{sunStatus && (
									<div class="settings-mono-box mt-1 text-[10px] leading-tight border-amber-500/30">
										<p class="text-amber-400 font-bold">{sunStatus}</p>
										<p class="text-gray-400">{sunProgress}</p>
									</div>
								)}
							</div>
						</div>

						{/* Koordinaten */}
						<div class="settings-section">
							<p class="settings-section-title">Koordinaten</p>
							<label class="flex items-center gap-2 cursor-pointer select-none">
								<input
									type="checkbox"
									checked={clickToCoordinatesMode}
									onChange={onToggleCoordinates}
									disabled={!tps}
									class="w-5 h-5 cursor-pointer accent-purple-500 disabled:cursor-not-allowed disabled:opacity-50"
								/>
								<span class="settings-label-text">Click to Coordinates Mode</span>
							</label>
							{lastCoordinates && (
								<div class="settings-mono-box">
									<p>mm: {lastCoordinates.mmX.toFixed(1)}, {lastCoordinates.mmY.toFixed(1)}</p>
									<p>lon/lat: {lastCoordinates.lon.toFixed(5)}, {lastCoordinates.lat.toFixed(5)}</p>
								</div>
							)}
						</div>

						{/* LED Suche */}
						<div class="settings-section">
							<p class="settings-section-title">LED Suche</p>
							<div class="flex gap-2">
								<input
									type="number"
									value={ledNumberInput}
									onChange={(e) => onLedInputChange((e.target as HTMLInputElement).value)}
									onKeyPress={(e: KeyboardEvent) => e.key === 'Enter' && onLedSearch()}
									placeholder="LED Index"
									disabled={!gridData}
									class="flex-1 bg-gray-700 text-white text-sm rounded-lg px-3 py-2 outline-none border border-gray-600 focus:border-blue-500 disabled:opacity-50"
								/>
								<button
									onClick={onLedSearch}
									disabled={!gridData}
									class="px-4 py-2 bg-cyan-600 text-white rounded-lg hover:bg-cyan-700 disabled:bg-gray-600 disabled:cursor-not-allowed text-sm font-semibold"
								>
									Suchen
								</button>
							</div>
							{ledNumberResult && (
								<div class="settings-mono-box">
									<p>LED #{ledNumberResult.index} (Col: {ledNumberResult.col}, Row: {ledNumberResult.row})</p>
									<p>mm: {ledNumberResult.mmX.toFixed(1)}, {ledNumberResult.mmY.toFixed(1)}</p>
									<p>lon/lat: {ledNumberResult.lon.toFixed(5)}, {ledNumberResult.lat.toFixed(5)}</p>
								</div>
							)}
						</div>

						{/* Design */}
						<div class="settings-section">
							<p class="settings-section-title">Design</p>
							<button onClick={onToggleDarkMode} class="settings-theme-btn">
								{darkMode ? 'Darkmode' : 'Lightmode'}
							</button>
						</div>

					</div>
				)}
			</div>
		</>
	);
}