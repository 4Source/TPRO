import { useState, useEffect, useRef } from 'preact/hooks';

function displayLabel(type: string): string {
	return type;
}

type Props = {
	effectsPath: string;
	// eslint-disable-next-line no-unused-vars
	onEffectChanged: (_path: string) => void;
};

export function QuickEffectPanel({ effectsPath, onEffectChanged }: Props) {
	const [open, setOpen] = useState(false);
	const [effects, setEffects] = useState<string[]>([]);
	const [selected, setSelected] = useState('');
	const [durationSec, setDurationSec] = useState(30);
	const [countdown, setCountdown] = useState(0);
	const [isRunning, setIsRunning] = useState(false);
	const intervalRef = useRef<ReturnType<typeof setInterval> | null>(null);
	const prevPathRef = useRef('');

	// Effektliste laden wenn Popover geöffnet wird
	useEffect(() => {
		if (!open) return;
		const dirPart = effectsPath.replace(/^\//, '');
		Promise.all([
			fetch(`/directory/${dirPart}/defaults`)
				.then(r => r.ok ? r.json() as Promise<string[]> : Promise.resolve([])),
			fetch(`/directory/${dirPart}`)
				.then(r => r.ok ? r.json() as Promise<string[]> : Promise.resolve([])),
		]).then(([defaultFiles, customFiles]) => {
			const defaults = (defaultFiles as string[])
				.filter(f => f.endsWith('.json'))
				.map(f => ({ label: f.replace('.json', ''), path: `${effectsPath}/defaults/${f}` }));
			const custom = (customFiles as string[])
				.filter(f => f.endsWith('.json'))
				.map(f => ({ label: f.replace('.json', ''), path: `${effectsPath}/${f}` }));
			const all = [...defaults, ...custom];
			setEffects(all.map(e => e.path));
			if (all.length > 0 && !selected) setSelected(all[0].path);
		}).catch(console.error);
	}, [open]);

	// Cleanup beim Unmount
	useEffect(() => {
		return () => { if (intervalRef.current) clearInterval(intervalRef.current); };
	}, []);

	const restorePrevious = async () => {
		if (intervalRef.current) {
			clearInterval(intervalRef.current);
			intervalRef.current = null;
		}
		const prev = prevPathRef.current;
		if (prev) {
			await fetch(`/config?current_effect=${prev}`, { method: 'PUT' })
				.catch(console.error);
			onEffectChanged(prev);
		}
		setIsRunning(false);
		setCountdown(0);
	};

	const startQuickEffect = async () => {
		if (!selected) return;

		// Aktuellen Effekt merken
		const res = await fetch('/config?current_effect').catch(() => null);
		const prev = res?.ok ? (await res.text()).trim().replace(/^"|"$/g, '') : '';
		prevPathRef.current = prev.startsWith('<') ? '' : prev;

		// Neuen Effekt setzen
		await fetch(`/config?current_effect=${selected}`, { method: 'PUT' })
			.catch(console.error);
		onEffectChanged(selected);

		// Countdown starten
		setOpen(false);
		setIsRunning(true);
		setCountdown(durationSec);

		let remaining = durationSec;
		intervalRef.current = setInterval(() => {
			remaining -= 1;
			setCountdown(remaining);
			if (remaining <= 0) {
				restorePrevious();
			}
		}, 1000);
	};

	if (isRunning) {
		return (
			<div class="quick-effect-running">
				<span class="quick-effect-countdown">{countdown}s</span>
				<button class="quick-effect-cancel" onClick={restorePrevious}>
					Abbrechen
				</button>
			</div>
		);
	}

	return (
		<div class="quick-effect-wrap">
			<button class="sim-toolbar-btn" onClick={() => setOpen(v => !v)}>
				Schneller Effekt
			</button>

			{open && (
				<>
					<div class="quick-effect-backdrop" onClick={() => setOpen(false)} />
					<div class="quick-effect-popover">
						<p class="quick-effect-label">Effekt</p>
						<select
							value={selected}
							onChange={(e) => setSelected((e.target as HTMLSelectElement).value)}
							class="quick-effect-select"
						>
							{effects.map(p => (
								<option key={p} value={p}>
									{displayLabel(p.split('/').pop()?.replace('.json', '') ?? p)}
								</option>
							))}
						</select>

						<p class="quick-effect-label">Dauer</p>
						<div class="quick-effect-duration-row">
							<input
								type="number"
								min="1"
								max="3600"
								value={durationSec}
								onInput={(e) => setDurationSec(parseInt((e.target as HTMLInputElement).value) || 30)}
								class="quick-effect-duration-input"
							/>
							<span class="quick-effect-unit">Sekunden</span>
						</div>

						<button class="quick-effect-start" onClick={startQuickEffect}>
							Starten
						</button>
					</div>
				</>
			)}
		</div>
	);
}
