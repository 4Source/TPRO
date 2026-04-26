import { ControlPanel } from '../../../components/ControlPanel/ControlPanel';
import type { Coordinates } from '../types';
import type { ThinPlateSpline } from '../../../tps';

type Props = {
	tps: ThinPlateSpline | null;
	active: boolean;
	lastCoordinates: Coordinates | null;
	onToggle: () => void;
};

export function CoordinatePanel({ tps, active, lastCoordinates, onToggle }: Props) {
	return (
		<ControlPanel class="flex items-center gap-3 px-6 py-4">
			<div class="flex items-center gap-2">
				<input
					type="checkbox"
					id="clickMode"
					checked={active}
					onChange={onToggle}
					disabled={!tps}
					class="w-5 h-5 cursor-pointer accent-purple-500 disabled:cursor-not-allowed disabled:opacity-50"
				/>
				<label for="clickMode" class="text-sm font-semibold text-white cursor-pointer select-none">
					🎯 Click to Coordinates Mode
				</label>
			</div>
			{lastCoordinates && (
				<div class="ml-auto text-xs text-gray-300 font-mono bg-gray-900 px-3 py-1.5 rounded">
					<div>mm: {lastCoordinates.mmX.toFixed(1)}, {lastCoordinates.mmY.toFixed(1)}</div>
					<div>lon/lat: {lastCoordinates.lon.toFixed(5)}, {lastCoordinates.lat.toFixed(5)}</div>
				</div>
			)}
		</ControlPanel>
	);
}
