import { ControlPanel } from '../../../components/ControlPanel/ControlPanel';
import { OpacitySlider } from '../../../components/OpacitySlider/OpacitySlider';

type Props = {
	showMap: boolean;
	mapOpacity: number;
	dotsOpacity: number;
	onToggleMap: () => void;

	onMapOpacityChange: (_value: number) => void;

	onDotsOpacityChange: (_value: number) => void;
};

export function VisibilityControls({ showMap, mapOpacity, dotsOpacity, onToggleMap, onMapOpacityChange, onDotsOpacityChange }: Props) {
	return (
		<ControlPanel class="flex items-center justify-between px-6 py-4">
			<div class="flex items-center gap-3">
				<input
					type="checkbox"
					id="worldMap"
					checked={showMap}
					onChange={onToggleMap}
					class="w-6 h-6 cursor-pointer accent-blue-500"
				/>
				<label for="worldMap" class="text-base font-semibold cursor-pointer select-none text-white">
					Show World Map Image
				</label>
			</div>
			<OpacitySlider label="Map Opacity" value={mapOpacity} accent="blue" disabled={!showMap} onChange={onMapOpacityChange} />
			<OpacitySlider label="Points Opacity" value={dotsOpacity} accent="purple" onChange={onDotsOpacityChange} />
		</ControlPanel>
	);
}
