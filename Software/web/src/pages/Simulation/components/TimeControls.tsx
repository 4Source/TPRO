import { ControlPanel } from '../../../components/ControlPanel/ControlPanel';

const SPEEDS = ['x1', 'x2', 'x10', 'x24', 'x240', 'x360', 'x3600'];

type Props = {
	selectedDate: string;
	timeValue: number;
	isPlaying: boolean;
	speed: string;

	// eslint-disable-next-line no-unused-vars
	onDateChange: (_value: string) => void;

	// eslint-disable-next-line no-unused-vars
	onTimeChange: (_value: number) => void;

	// eslint-disable-next-line no-unused-vars
	onPlayingChange: (_value: boolean) => void;

	// eslint-disable-next-line no-unused-vars
	onSpeedChange: (_value: string) => void;
};

function formatTime(minutes: number): string {
	const hrs = Math.floor(minutes / 60);
	const mins = minutes % 60;
	return `${hrs.toString().padStart(2, '0')}:${mins.toString().padStart(2, '0')}`;
}

export function TimeControls({ selectedDate, timeValue, isPlaying, speed, onDateChange, onTimeChange, onPlayingChange, onSpeedChange }: Props) {
	return (
		<ControlPanel class="flex items-center gap-4 p-4 w-full">
			<div class="flex items-center gap-2 min-w-[160px]">
				<input
					type="date"
					value={selectedDate}
					onChange={(e) => onDateChange((e.target as HTMLInputElement).value)}
					class="bg-gray-700 text-white text-sm rounded-lg px-3 py-2 outline-none border border-gray-600 focus:border-blue-500 w-full"
				/>
			</div>

			<div class="flex flex-grow items-center gap-4 px-2">
				<span class="text-lg font-mono text-blue-400 min-w-[55px] text-center">{formatTime(timeValue)}</span>
				<div class="slider-container">
					<div class="slider-horizontal-line" />
					<input
						type="range"
						min="0"
						max="1439"
						value={timeValue}
						onInput={(e) => onTimeChange(parseInt((e.target as HTMLInputElement).value, 10))}
					/>
				</div>
			</div>

			<div class="flex items-center gap-2">
				<div class="flex bg-gray-900 rounded-lg p-1 border border-gray-700">
					<button
						onClick={() => onPlayingChange(true)}
						class={`px-4 py-1.5 rounded-md text-xs font-bold transition-all ${isPlaying ? 'bg-slate-200 text-black shadow-inner' : 'text-gray-400 hover:text-white'}`}
					>
						START
					</button>
					<button
						onClick={() => onPlayingChange(false)}
						class={`px-4 py-1.5 rounded-md text-xs font-bold transition-all ${!isPlaying ? 'bg-slate-200 text-black shadow-inner' : 'text-gray-400 hover:text-white'}`}
					>
						STOP
					</button>
				</div>
				<select
					value={speed}
					onChange={(e) => onSpeedChange((e.target as HTMLSelectElement).value)}
					class="bg-gray-700 text-white text-sm rounded-lg px-3 py-2 outline-none border border-gray-600 focus:border-blue-500 min-w-[80px]"
				>
					{SPEEDS.map(s => <option key={s} value={s}>{s}</option>)}
				</select>
			</div>
		</ControlPanel>
	);
}
