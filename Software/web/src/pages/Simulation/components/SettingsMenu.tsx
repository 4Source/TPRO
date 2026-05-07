import { useState } from 'preact/hooks';
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

	// eslint-disable-next-line no-unused-vars
	onFileUpload: (_e: Event) => void;
	onCalculate: () => void;
	onExport: () => void;
	clickToCoordinatesMode: boolean;
	lastCoordinates: Coordinates | null;
	onToggleCoordinates: () => void;
	ledNumberInput: string;
	ledNumberResult: LedData | null;

	// eslint-disable-next-line no-unused-vars
	onLedInputChange: (_value: string) => void;
	onLedSearch: () => void;
	showMap: boolean;
	mapOpacity: number;
	dotsOpacity: number;
	onToggleMap: () => void;

	// eslint-disable-next-line no-unused-vars
	onMapOpacityChange: (_value: number) => void;

	// eslint-disable-next-line no-unused-vars
	onDotsOpacityChange: (_value: number) => void;
};

export function SettingsMenu({
	open, onClose,
	darkMode, onToggleDarkMode,
	tps, isCalculating, gridData, sourcePoints, onFileUpload, onCalculate, onExport,
	clickToCoordinatesMode, lastCoordinates, onToggleCoordinates,
	ledNumberInput, ledNumberResult, onLedInputChange, onLedSearch,
	showMap, mapOpacity, dotsOpacity, onToggleMap, onMapOpacityChange, onDotsOpacityChange,
}: Props) {
	const [showEffects, setShowEffects] = useState(false);

	const [sunStatus, setSunStatus] = useState<string | null>(null);
	const [sunProgress, setSunProgress] = useState("");

	const handleGenerateSunData = () => {
		if (!gridData) return;

		// Mapping deiner LedData auf das Format der API-Funktion
		const leds = gridData.map(led => ({
			index: led.index,
			x: led.col,
			y: led.row,
			lat: led.lat,
			lon: led.lon
		}));

		setSunStatus("Starte...");
		startSunDataGeneration(leds, 2024, (status, progress) => {
			setSunStatus(status);
			setSunProgress(progress);
		});
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
						<EffectPanel />
					</div>
				) : (
					<div class="settings-panel-body">

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
										disabled={!gridData || sunStatus !== null && sunStatus !== "Abgeschlossen"}
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
						</div>

						{/* Effekt */}
						<div class="settings-section">
							<p class="settings-section-title">Effekt</p>
							<button class="settings-nav-btn" onClick={() => setShowEffects(true)}>
								Effekt auswählen
								<span class="settings-nav-arrow">→</span>
							</button>
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
