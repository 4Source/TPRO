type Props = {
	label: string;
	value: number;
	accent?: 'blue' | 'purple';
	disabled?: boolean;

	onChange: (_value: number) => void;
};

export function OpacitySlider({ label, value, accent = 'blue', disabled = false, onChange }: Props) {
	return (
		<div class="flex items-center gap-3 bg-gray-900 px-4 py-1.5 rounded-lg border border-gray-700">
			<span class="text-xs text-gray-400 uppercase tracking-wider font-bold">{label}</span>
			<input
				type="range"
				min="0"
				max="1"
				step="0.01"
				value={value}
				onInput={(e) => onChange(parseFloat((e.target as HTMLInputElement).value))}
				class={`w-32 h-1.5 cursor-pointer ${accent === 'blue' ? 'accent-blue-500' : 'accent-purple-500'}`}
				disabled={disabled}
			/>
			<span class="text-xs font-mono w-8 text-right text-white">{Math.round(value * 100)}%</span>
		</div>
	);
}
