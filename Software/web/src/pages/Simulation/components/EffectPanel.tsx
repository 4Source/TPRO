import { useState, useEffect } from 'preact/hooks';

// ── Types ────────────────────────────────────────────────────────────────────

type ParamSchema = {
	type: 'number' | 'color' | 'string' | 'range' | 'timeline';
	min?: number;
	max?: number;
	step?: number;
};

type EffectSchema = Record<string, ParamSchema>;
type ParamValue = number | string | number[];
type EffectParams = Record<string, ParamValue>;

type SubeffectConfig = { type: string; path: string };

// 137-branch flat JSON format: type/parameters/schema sit at the root, not under an "effect" wrapper
type EffectJson = {
	version?: string;
	name?: string;
	type: string;
	parameters: EffectParams;
	schema: EffectSchema;
	subeffects?: SubeffectConfig[];
};

type EffectEntry = {
	filename: string;
	type: string;
	path: string;
	isDefault: boolean;
	effectJsonType: string; // actual "type" field from the JSON file
};

// ── Helpers ──────────────────────────────────────────────────────────────────

function hexToRgb(hex: string): number[] {
	const n = parseInt(hex.replace('#', ''), 16);
	return [(n >> 16) & 255, (n >> 8) & 255, n & 255];
}

function rgbToHex(rgb: number[]): string {
	return '#' + rgb.map(v => Math.max(0, Math.min(255, Math.round(v))).toString(16).padStart(2, '0')).join('');
}

function deepCopy<T>(obj: T): T {
	return JSON.parse(JSON.stringify(obj)) as T;
}

function safePathText(text: string, fallback: string): string {
	const t = text.trim().replace(/^"|"$/g, '');
	if (t.startsWith('<') || t.startsWith('{') || t.startsWith('[') || t === '') return fallback;
	return t;
}

function prettyLabel(key: string): string {
	return key.replace(/_/g, ' ').replace(/\b\w/g, c => c.toUpperCase());
}

function displayLabel(type: string): string {
	return type;
}

// ── ParamField ───────────────────────────────────────────────────────────────

type ParamFieldProps = {
	name: string;
	value: ParamValue;
	schema: ParamSchema;
	onChange: (_v: ParamValue) => void;
};

function ParamField({ name, value, schema, onChange }: ParamFieldProps) {
	const label = prettyLabel(name);

	if (schema.type === 'color') {
		const rgb = Array.isArray(value) ? (value as number[]) : [255, 255, 255];
		return (
			<div class="param-row">
				<label class="param-label">{label}</label>
				<div class="param-color-row">
					<input
						type="color"
						value={rgbToHex(rgb)}
						onInput={(e) => onChange(hexToRgb((e.target as HTMLInputElement).value))}
						class="param-color-input"
					/>
					<span class="param-value-text">rgb({rgb[0]}, {rgb[1]}, {rgb[2]})</span>
				</div>
			</div>
		);
	}

	if (schema.type === 'number') {
		const num = typeof value === 'number' ? value : 0;
		const min = schema.min ?? 0;
		const max = schema.max ?? 100;
		const step = schema.step && schema.step > 0 ? schema.step : 1;
		return (
			<div class="param-row">
				<label class="param-label">{label}</label>
				<div class="param-slider-row">
					<input
						type="range"
						min={min} max={max} step={step} value={num}
						onInput={(e) => onChange(Number((e.target as HTMLInputElement).value))}
						class="param-slider"
					/>
					<input
						type="number"
						min={min} max={max} step={step} value={num}
						onInput={(e) => onChange(Number((e.target as HTMLInputElement).value))}
						class="param-value-input"
					/>
				</div>
			</div>
		);
	}

	if (schema.type === 'range') {
		const arr = Array.isArray(value) ? (value as number[]) : [0, 1000];
		const min = schema.min ?? 0;
		const max = schema.max ?? 3024;
		return (
			<div class="param-row">
				<label class="param-label">{label}</label>
				<div class="param-range-row">
					<span class="param-range-label">Start</span>
					<input
						type="number" min={min} max={arr[1]} value={arr[0]}
						onInput={(e) => onChange([Number((e.target as HTMLInputElement).value), arr[1]])}
						class="param-range-input"
					/>
					<span class="param-range-label">Ende</span>
					<input
						type="number" min={arr[0]} max={max} value={arr[1]}
						onInput={(e) => onChange([arr[0], Number((e.target as HTMLInputElement).value)])}
						class="param-range-input"
					/>
				</div>
			</div>
		);
	}

	if (schema.type === 'string') {
		return (
			<div class="param-row">
				<label class="param-label">{label}</label>
				<input
					type="text"
					value={String(value)}
					onInput={(e) => onChange((e.target as HTMLInputElement).value)}
					class="param-text-input"
				/>
			</div>
		);
	}

	return null;
}

// ── TimelineEditor ───────────────────────────────────────────────────────────

type TimelineEditorProps = {
	effectJson: EffectJson;
	availableEffects: EffectEntry[];
	onChange: (_updated: EffectJson) => void;
};

function TimelineEditor({ effectJson, availableEffects, onChange }: TimelineEditorProps) {
	const subeffects = effectJson.subeffects ?? [];
	const primary = effectJson.parameters['primary'] as { id: number; cycle_time: number } | undefined;
	const secondary = (effectJson.parameters['secondary'] ?? []) as Array<{ id: number; cycle_time: number }>;

	const steps = subeffects.map((sub, i) => {
		let duration = 1000;
		if (primary && i === primary.id) duration = primary.cycle_time;
		else {
			const sec = secondary.find(s => s.id === i);
			if (sec) duration = sec.cycle_time;
		}
		return { path: sub.path, duration_ms: duration };
	});

	const nonTimeline = availableEffects.filter(e => e.effectJsonType !== 'timeline');

	const updateSteps = (newSteps: Array<{ path: string; duration_ms: number }>) => {
		onChange({
			...effectJson,
			subeffects: newSteps.map(s => ({
				type: s.path.split('/').pop()?.replace('.json', '') ?? 'unknown',
				path: s.path,
			})),
			parameters: {
				...effectJson.parameters,
				primary: { id: 0, cycle_time: newSteps[0]?.duration_ms ?? 1000, overwrite: {} },
				secondary: newSteps.slice(1).map((s, i) => ({
					id: i + 1,
					cycle_time: s.duration_ms,
					overwrite: {},
				})),
			},
		});
	};

	const addStep = () => {
		const first = nonTimeline[0];
		if (!first) return;
		updateSteps([...steps, { path: first.path, duration_ms: 3000 }]);
	};

	const removeStep = (i: number) => {
		if (steps.length <= 1) return;
		updateSteps(steps.filter((_, idx) => idx !== i));
	};

	const updateStep = (i: number, field: 'path' | 'duration_ms', val: string | number) => {
		const next = [...steps];
		next[i] = { ...next[i], [field]: val };
		updateSteps(next);
	};

	const totalSec = steps.reduce((s, step) => s + step.duration_ms, 0) / 1000;

	return (
		<>
			<p class="settings-section-title">
				Schritte
				<span class="effect-duration-total"> · Gesamt: {totalSec.toFixed(1)} s</span>
			</p>
			<div class="timeline-steps">
				{steps.map((step, i) => (
					<div key={i} class="timeline-step">
						<span class="timeline-step-num">{i + 1}</span>
						<select
							value={step.path}
							onChange={(e) => updateStep(i, 'path', (e.target as HTMLSelectElement).value)}
							class="timeline-step-select"
						>
							{nonTimeline.map(e => (
								<option key={e.path} value={e.path}>{displayLabel(e.type)}</option>
							))}
						</select>
						<input
							type="number" min="100" step="500"
							value={step.duration_ms}
							onChange={(e) => updateStep(i, 'duration_ms', parseInt((e.target as HTMLInputElement).value) || 1000)}
							class="timeline-step-duration"
						/>
						<span class="timeline-step-unit">ms</span>
						<button
							class="timeline-step-remove"
							onClick={() => removeStep(i)}
							disabled={steps.length <= 1}
						>✕</button>
					</div>
				))}
				<button class="timeline-add-btn" onClick={addStep}>+ Schritt hinzufügen</button>
			</div>
		</>
	);
}

// ── DeleteConfirmDialog ──────────────────────────────────────────────────────

type DeleteConfirmDialogProps = {
	entry: EffectEntry;
	onConfirm: () => void;
	onCancel: () => void;
};

function DeleteConfirmDialog({ entry, onConfirm, onCancel }: DeleteConfirmDialogProps) {
	return (
		<div class="save-dialog-backdrop" onClick={onCancel}>
			<div class="save-dialog" onClick={(e) => e.stopPropagation()}>
				<p class="save-dialog-title">Effekt löschen</p>
				<p class="delete-confirm-text">
					Möchten Sie den Effekt <strong>{entry.type}</strong> wirklich löschen?
				</p>
				<div class="save-dialog-actions">
					<button class="save-dialog-cancel" onClick={onCancel}>Abbrechen</button>
					<button class="effect-delete-confirm-btn" onClick={onConfirm}>Löschen</button>
				</div>
			</div>
		</div>
	);
}

// ── SaveDialog ───────────────────────────────────────────────────────────────

type SaveDialogProps = {
	onSave: (_name: string) => Promise<void>;
	onClose: () => void;
};

function SaveDialog({ onSave, onClose }: SaveDialogProps) {
	const [name, setName] = useState('');
	const [saving, setSaving] = useState(false);
	const [error, setError] = useState('');

	const handleSave = async () => {
		const trimmed = name.trim();
		if (!trimmed) { setError('Bitte einen Namen eingeben.'); return; }
		setSaving(true);
		setError('');
		try {
			await onSave(trimmed);
			onClose();
		} catch (e) {
			setError(e instanceof Error ? e.message : 'Fehler beim Speichern');
			setSaving(false);
		}
	};

	return (
		<div class="save-dialog-backdrop" onClick={onClose}>
			<div class="save-dialog" onClick={(e) => e.stopPropagation()}>
				<p class="save-dialog-title">Als neuer Effekt speichern</p>

				<div class="save-dialog-field">
					<input
						type="text"
						placeholder="Effektname"
						value={name}
						onInput={(e) => { setName((e.target as HTMLInputElement).value); setError(''); }}
						onKeyDown={(e) => { if (e.key === 'Enter') handleSave(); if (e.key === 'Escape') onClose(); }}
						class="save-dialog-input"
						// eslint-disable-next-line jsx-a11y/no-autofocus
						autoFocus
					/>
				</div>

				{name.trim() && (
					<p class="save-dialog-hint">Dateiname: {name.trim()}.json</p>
				)}

				{error && <p class="save-dialog-error">{error}</p>}

				<div class="save-dialog-actions">
					<button class="save-dialog-cancel" onClick={onClose} disabled={saving}>
						Abbrechen
					</button>
					<button class="save-dialog-save" onClick={handleSave} disabled={saving || !name.trim()}>
						{saving ? 'Speichern…' : 'Speichern'}
					</button>
				</div>
			</div>
		</div>
	);
}

// ── EffectPanel ──────────────────────────────────────────────────────────────

type EffectPanelProps = {
	// eslint-disable-next-line no-unused-vars
	onEffectActivated?: (_path: string) => void;
};

export function EffectPanel({ onEffectActivated }: EffectPanelProps = {}) {
	const [effectsPath, setEffectsPath] = useState('/effects');
	const [entries, setEntries] = useState<EffectEntry[]>([]);
	const [activeEffectPath, setActiveEffectPath] = useState('');
	const [selected, setSelected] = useState<EffectEntry | null>(null);
	const [loadedJson, setLoadedJson] = useState<EffectJson | null>(null);
	const [editedJson, setEditedJson] = useState<EffectJson | null>(null);
	const [isLoadingEffect, setIsLoadingEffect] = useState(false);
	const [applyStatus, setApplyStatus] = useState<'idle' | 'saving' | 'success' | 'error'>('idle');
	const [applyError, setApplyError] = useState('');
	const [showSaveDialog, setShowSaveDialog] = useState(false);
	const [deleteCandidate, setDeleteCandidate] = useState<EffectEntry | null>(null);

	// ── Load list ─────────────────────────────────────────────────────────────

	const loadList = (base: string) => {
		const dirPart = base.replace(/^\//, '');
		return Promise.all([
			fetch(`/directory/${dirPart}/defaults`)
				.then(r => r.ok ? r.json() as Promise<string[]> : Promise.resolve([])),
			fetch(`/directory/${dirPart}`)
				.then(r => r.ok ? r.json() as Promise<string[]> : Promise.resolve([])),
		]).then(([defaultFiles, customFiles]) => {
			const defaults: EffectEntry[] = (defaultFiles as string[])
				.filter(f => f.endsWith('.json'))
				.map(f => ({ filename: f, type: f.replace('.json', ''), path: `${base}/defaults/${f}`, isDefault: true, effectJsonType: f.replace('.json', '') }));

			const customFilenames = (customFiles as string[]).filter(f => f.endsWith('.json'));
			return Promise.all(
				customFilenames.map(f => {
					const path = `${base}/${f}`;
					return fetch(`/file/${path.replace(/^\//, '')}`)
						.then(r => r.ok ? r.json() as Promise<{ type?: string }> : Promise.resolve({}))
						.then((json: { type?: string }) => ({
							filename: f, type: f.replace('.json', ''), path, isDefault: false,
							effectJsonType: json.type ?? f.replace('.json', ''),
						} as EffectEntry))
						.catch(() => ({ filename: f, type: f.replace('.json', ''), path, isDefault: false, effectJsonType: f.replace('.json', '') } as EffectEntry));
				})
			).then(custom => {
				const list = [...defaults, ...custom];
				setEntries(list);
				return list;
			});
		});
	};

	useEffect(() => {
		Promise.all([
			fetch('/config?effects_path').then(r => r.ok ? r.text() : '/effects').catch(() => '/effects'),
			fetch('/config?current_effect').then(r => r.ok ? r.text() : '').catch(() => ''),
		]).then(([ePath, activePath]) => {
			const base = safePathText(ePath, '/effects');
			setEffectsPath(base);
			const active = safePathText(activePath, '');
			setActiveEffectPath(active);

			return loadList(base).then(list => {
				const activeEntry = list.find(e => e.path === active);
				if (activeEntry) loadEntry(activeEntry);
			});
		}).catch(console.error);
	}, []);

	// ── Load effect details ───────────────────────────────────────────────────

	const loadEntry = (entry: EffectEntry) => {
		setSelected(entry);
		setLoadedJson(null);
		setEditedJson(null);
		setIsLoadingEffect(true);

		const filePart = entry.path.replace(/^\//, '');
		fetch(`/file/${filePart}`)
			.then(r => r.ok ? r.json() as Promise<EffectJson> : Promise.resolve(null))
			.then((json: EffectJson | null) => {
				setLoadedJson(json);
				setEditedJson(json ? deepCopy(json) : null);
				setIsLoadingEffect(false);
			})
			.catch(() => setIsLoadingEffect(false));
	};

	// ── Param changes ─────────────────────────────────────────────────────────

	const handleParamChange = (key: string, val: ParamValue) => {
		if (!editedJson) return;
		setEditedJson({
			...editedJson,
			parameters: { ...editedJson.parameters, [key]: val },
		});
	};

	// ── Effekt löschen ───────────────────────────────────────────────────────

	const confirmDelete = async () => {
		if (!deleteCandidate) return;
		const filePart = deleteCandidate.path.replace(/^\//, '');
		const r = await fetch(`/file/${filePart}`, { method: 'DELETE' }).catch(() => null);
		if (r && !r.ok) return; // Fehler still ignorieren — Liste trotzdem bereinigen
		setEntries(prev => prev.filter(e => e.path !== deleteCandidate.path));
		if (selected?.path === deleteCandidate.path) {
			setSelected(null);
			setLoadedJson(null);
			setEditedJson(null);
		}
		if (activeEffectPath === deleteCandidate.path) setActiveEffectPath('');
		setDeleteCandidate(null);
	};

	// ── Anwenden: nur aktivieren, Originaldatei bleibt unberührt ─────────────

	const applyEffect = async () => {
		if (!selected || !loadedJson) return;
		setApplyStatus('saving');
		setApplyError('');
		try {
			const r = await fetch(
				`/config?current_effect=${selected.path}`,
				{ method: 'PUT' },
			);
			if (!r.ok) throw new Error(`HTTP ${r.status}`);
			setActiveEffectPath(selected.path);
			onEffectActivated?.(selected.path);
			setApplyStatus('success');
			setTimeout(() => setApplyStatus('idle'), 2500);
		} catch (e) {
			setApplyStatus('error');
			setApplyError(e instanceof Error ? e.message : 'Fehler');
			setTimeout(() => setApplyStatus('idle'), 3000);
		}
	};

	// ── Als neuer Effekt speichern ────────────────────────────────────────────

	const saveAsNew = async (finalName: string) => {
		if (!loadedJson || !editedJson) throw new Error('Kein Effekt geladen');

		const newPath = `${effectsPath}/${finalName}.json`;
		const toSave: EffectJson = {
			...loadedJson,
			name: finalName,
			parameters: editedJson.parameters,
			...(editedJson.subeffects !== undefined ? { subeffects: editedJson.subeffects } : {}),
		};

		const filePart = newPath.replace(/^\//, '');
		const r1 = await fetch(`/effect/${filePart}`, {
			method: 'PUT',
			body: JSON.stringify(toSave),
		});
		if (!r1.ok) throw new Error(`Speichern fehlgeschlagen: HTTP ${r1.status}`);

		const r2 = await fetch(`/config?current_effect=${newPath}`, { method: 'PUT' });
		if (!r2.ok) throw new Error(`Aktivieren fehlgeschlagen: HTTP ${r2.status}`);

		// Liste neu laden und neuen Eintrag auswählen
		const newList = await loadList(effectsPath);
		setActiveEffectPath(newPath);
		onEffectActivated?.(newPath);
		const newEntry = newList.find(e => e.path === newPath);
		if (newEntry) loadEntry(newEntry);
	};

	// ── Render ────────────────────────────────────────────────────────────────

	const isTimeline = loadedJson?.type === 'timeline';
	const schema = loadedJson?.schema ?? {};
	const editableParams = Object.entries(schema).filter(([, s]) => s.type !== 'timeline');

	const applyLabel =
		applyStatus === 'saving' ? 'Wird aktiviert…' :
		applyStatus === 'success' ? 'Aktiviert' :
		applyStatus === 'error' ? `Fehler: ${applyError}` :
		'Anwenden';

	return (
		<div class="effect-panel">

			{/* Effektliste */}
			<div class="settings-section">
				<p class="settings-section-title">Effekt</p>
				{entries.length === 0 ? (
					<p class="settings-hint">Suche Effekte in {effectsPath}…</p>
				) : (
					<div class="effect-cards">
						{entries.map(e => (
							<div key={e.path} class="effect-card-row">
								<button
									class={`effect-card${selected?.path === e.path ? ' selected' : ''}${activeEffectPath === e.path ? ' active' : ''}`}
									onClick={() => loadEntry(e)}
								>
									<span class="effect-card-label">{displayLabel(e.type)}</span>
									{activeEffectPath === e.path && <span class="effect-active-badge">Aktiv</span>}
								</button>
								{!e.isDefault && (
									<button
										class="effect-delete-btn"
										onClick={() => setDeleteCandidate(e)}
										title={`${e.type} löschen`}
									>✕</button>
								)}
							</div>
						))}
					</div>
				)}
			</div>

			{/* Parameter / Timeline-Editor */}
			{selected && (
				<>
					{isLoadingEffect && (
						<div class="settings-section">
							<p class="settings-hint">Lade Parameter…</p>
						</div>
					)}

					{!isLoadingEffect && editedJson && !isTimeline && editableParams.length > 0 && (
						<div class="settings-section">
							<p class="settings-section-title">Parameter</p>
							<div class="effect-params">
								{editableParams.map(([key, s]) => (
									<ParamField
										key={key}
										name={key}
										value={editedJson.parameters[key] ?? 0}
										schema={s}
										onChange={(v) => handleParamChange(key, v)}
									/>
								))}
							</div>
						</div>
					)}

					{!isLoadingEffect && editedJson && isTimeline && (
						<div class="settings-section">
							<TimelineEditor
								effectJson={editedJson}
								availableEffects={entries}
								onChange={(updated) => setEditedJson(updated)}
							/>
						</div>
					)}

					<div class="effect-apply-row">
						<button
							class={`effect-apply-btn${applyStatus !== 'idle' ? ` ${applyStatus}` : ''}`}
							onClick={applyEffect}
							disabled={applyStatus === 'saving' || !loadedJson}
						>
							{applyLabel}
						</button>
						<button
							class="effect-save-new-btn"
							onClick={() => setShowSaveDialog(true)}
							disabled={!loadedJson || !editedJson}
						>
							Als neuer Effekt speichern
						</button>
					</div>
				</>
			)}

			{/* Speichern-Dialog */}
			{showSaveDialog && (
				<SaveDialog
					onSave={saveAsNew}
					onClose={() => setShowSaveDialog(false)}
				/>
			)}

			{/* Löschen-Bestätigung */}
			{deleteCandidate && (
				<DeleteConfirmDialog
					entry={deleteCandidate}
					onConfirm={confirmDelete}
					onCancel={() => setDeleteCandidate(null)}
				/>
			)}
		</div>
	);
}
