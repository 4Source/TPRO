import { ControlPanel } from '../../../components/ControlPanel/ControlPanel';
import type { LedData } from '../types';

type Props = {
	gridData: LedData[] | null;
	inputValue: string;
	result: LedData | null;

	onInputChange: (_value: string) => void;
	onSearch: () => void;
};

export function LedLookupPanel({ gridData, inputValue, result, onInputChange, onSearch }: Props) {
	const handleKeyPress = (e: KeyboardEvent) => {
		if (e.key === 'Enter') onSearch();
	};

	return (
		<ControlPanel class="flex items-center gap-3 px-6 py-4">
			<label class="text-sm font-semibold text-white">🔍 LED Nummer:</label>
			<input
				type="number"
				value={inputValue}
				onChange={(e) => onInputChange((e.target as HTMLInputElement).value)}
				onKeyPress={handleKeyPress}
				placeholder="LED Index"
				disabled={!gridData}
				class="bg-gray-700 text-white text-sm rounded-lg px-3 py-2 outline-none border border-gray-600 focus:border-blue-500 w-32 disabled:opacity-50"
			/>
			<button
				onClick={onSearch}
				disabled={!gridData}
				class="px-4 py-2 bg-cyan-600 text-white rounded-lg hover:bg-cyan-700 disabled:bg-gray-600 disabled:cursor-not-allowed font-semibold text-sm"
			>
				Suchen
			</button>
			{result && (
				<div class="ml-auto text-xs text-gray-300 font-mono bg-gray-900 px-3 py-1.5 rounded">
					<div>LED #{result.index} (Col: {result.col}, Row: {result.row})</div>
					<div>mm: {result.mmX.toFixed(1)}, {result.mmY.toFixed(1)}</div>
					<div>lon/lat: {result.lon.toFixed(5)}, {result.lat.toFixed(5)}</div>
				</div>
			)}
		</ControlPanel>
	);
}
