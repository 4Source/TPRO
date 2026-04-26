import { useState, useEffect } from 'preact/hooks';

type PrimaryType = 'blinking' | 'day_night' | 'timeline';
type SecondaryType = 'blinking' | 'day_night';

interface TimelineStep {
	id: number;
	effect: SecondaryType;
	duration_ms: number;
}

const PRIMARY_EFFECTS: { id: PrimaryType; label: string; description: string }[] = [
	{ id: 'blinking', label: 'Blinken', description: '1 s an · 1 s aus' },
	{ id: 'day_night', label: 'Tag / Nacht', description: 'Basiert auf Tageszeit' },
	{ id: 'timeline', label: 'Timeline', description: 'Sekundäreffekte in Sequenz' },
];

const SECONDARY_LABELS: Record<SecondaryType, string> = {
	blinking: 'Blinken',
	day_night: 'Tag / Nacht',
};

function detectType(path: string): PrimaryType | null {
	if (path.includes('blinking')) return 'blinking';
	if (path.includes('day_night')) return 'day_night';
	if (path.includes('timeline')) return 'timeline';
	return null;
}

function getButtonLabel(status: string, errorMsg: string): string {
	if (status === 'applying') return 'Wird angewendet…';
	if (status === 'success') return 'Erfolgreich angewendet';
	if (status === 'error') return `Fehler: ${errorMsg}`;
	return 'Anwenden';
}

export function EffectPanel() {
	const [effectsPath, setEffectsPath] = useState('/effects');
	const [activeType, setActiveType] = useState<PrimaryType | null>(null);
	const [selected, setSelected] = useState<PrimaryType | null>(null);
	const [status, setStatus] = useState<'idle' | 'applying' | 'success' | 'error'>('idle');
	const [errorMsg, setErrorMsg] = useState('');
	const [steps, setSteps] = useState<TimelineStep[]>([{ id: 1, effect: 'blinking', duration_ms: 5000 }]);
	const [nextId, setNextId] = useState(2);

	useEffect(() => {
		Promise.all([
			fetch('/config?current_effect').then(r => r.text()).catch(() => ''),
			fetch('/config?effects_path').then(r => r.text()).catch(() => '/effects'),
		]).then(([path, ePath]) => {
			const base = ePath.trim() || '/effects';
			setEffectsPath(base);
			const type = detectType(path.trim());
			setActiveType(type);
			setSelected(type);
		});
	}, []);

	const applyEffect = async () => {
		if (!selected) return;
		setStatus('applying');
		setErrorMsg('');
		const path = `${effectsPath}/${selected}.json`;
		try {
			const res = await fetch(`/config?current_effect=${encodeURIComponent(path)}`, { method: 'PUT' });
			if (res.ok) {
				setActiveType(selected);
				setStatus('success');
				setTimeout(() => setStatus('idle'), 2500);
			}
			else {
				throw new Error(`HTTP ${res.status}`);
			}
		}
		catch (e) {
			setStatus('error');
			setErrorMsg(e instanceof Error ? e.message : 'Unbekannter Fehler');
			setTimeout(() => setStatus('idle'), 3000);
		}
	};

	const addStep = () => {
		setSteps(prev => [...prev, { id: nextId, effect: 'blinking', duration_ms: 5000 }]);
		setNextId(n => n + 1);
	};

	const removeStep = (id: number) => setSteps(prev => prev.filter(s => s.id !== id));

	const updateStep = (id: number, field: keyof Omit<TimelineStep, 'id'>, value: string | number) => {
		setSteps(prev => prev.map(s => (s.id === id ? { ...s, [field]: value } : s)));
	};

	const totalDurationSec = steps.reduce((sum, s) => sum + s.duration_ms, 0) / 1000;

	return (
		<div class="effect-panel">

			{/* Primäreffekte */}
			<div class="settings-section">
				<p class="settings-section-title">Primäreffekt</p>
				<div class="effect-cards">
					{PRIMARY_EFFECTS.map(eff => (
						<button
							key={eff.id}
							class={`effect-card${selected === eff.id ? ' selected' : ''}${activeType === eff.id ? ' active' : ''}`}
							onClick={() => setSelected(eff.id)}
						>
							<span class="effect-card-label">{eff.label}</span>
							<span class="effect-card-desc">{eff.description}</span>
							{activeType === eff.id && <span class="effect-active-badge">Aktiv</span>}
						</button>
					))}
				</div>
			</div>

			{/* Sekundäreffekte (Timeline-Schritte) */}
			{selected === 'timeline' && (
				<div class="settings-section">
					<p class="settings-section-title">
						Sekundäreffekte
						<span class="effect-duration-total"> · Gesamt: {totalDurationSec.toFixed(1)} s</span>
					</p>
					<div class="timeline-steps">
						{steps.map((step, i) => (
							<div key={step.id} class="timeline-step">
								<span class="timeline-step-num">{i + 1}</span>
								<select
									value={step.effect}
									onChange={(e) => updateStep(step.id, 'effect', (e.target as HTMLSelectElement).value)}
									class="timeline-step-select"
								>
									{(Object.keys(SECONDARY_LABELS) as SecondaryType[]).map(k => (
										<option key={k} value={k}>{SECONDARY_LABELS[k]}</option>
									))}
								</select>
								<input
									type="number"
									min="100"
									step="500"
									value={step.duration_ms}
									onChange={(e) => updateStep(step.id, 'duration_ms', parseInt((e.target as HTMLInputElement).value, 10) || 1000)}
									class="timeline-step-duration"
								/>
								<span class="timeline-step-unit">ms</span>
								<button
									class="timeline-step-remove"
									onClick={() => removeStep(step.id)}
									disabled={steps.length <= 1}
								>✕</button>
							</div>
						))}
						<button class="timeline-add-btn" onClick={addStep}>+ Schritt hinzufügen</button>
					</div>
				</div>
			)}

			{/* Anwenden */}
			<div class="effect-apply-row">
				<button
					class={`effect-apply-btn${status !== 'idle' ? ` ${status}` : ''}`}
					onClick={applyEffect}
					disabled={!selected || status === 'applying'}
				>
					{getButtonLabel(status, errorMsg)}
				</button>
			</div>
		</div>
	);
}
