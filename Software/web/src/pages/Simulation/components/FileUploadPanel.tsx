import { ControlPanel } from '../../../components/ControlPanel/ControlPanel';
import type { LedData } from '../types';
import type { ThinPlateSpline } from '../../../tps';

type Props = {
	tps: ThinPlateSpline | null;
	isCalculating: boolean;
	gridData: LedData[] | null;
	sourcePoints: number[][];

	onFileUpload: (_e: Event) => void;
	onCalculate: () => void;
	onExport: () => void;
};

export function FileUploadPanel({ tps, isCalculating, gridData, sourcePoints, onFileUpload, onCalculate, onExport }: Props) {
	return (
		<ControlPanel class="flex items-center gap-3 px-6 py-4">
			<label class="text-sm font-semibold text-white cursor-pointer">
				📤 Punkte laden (.csv):
				<input type="file" accept=".csv,.txt" onChange={onFileUpload} class="hidden" />
			</label>
			<button
				onClick={onCalculate}
				disabled={!tps || isCalculating}
				class="px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 disabled:bg-gray-600 disabled:cursor-not-allowed font-semibold"
			>
				{isCalculating ? 'Berechne...' : 'Raster berechnen'}
			</button>
			<button
				onClick={onExport}
				disabled={!gridData}
				class="px-4 py-2 bg-green-600 text-white rounded-lg hover:bg-green-700 disabled:bg-gray-600 disabled:cursor-not-allowed font-semibold"
			>
				💾 Exportieren
			</button>
			{sourcePoints.length > 0 && (
				<span class="text-xs text-gray-400">
					({sourcePoints.length} Ankerpunkte, {gridData?.length ?? 0} LEDs)
				</span>
			)}
		</ControlPanel>
	);
}
