import type { ComponentChildren } from 'preact';

type Props = {
	children: ComponentChildren;
	class?: string;
};

export function ControlPanel({ children, class: extraClass = '' }: Props) {
	return (
		<div class={`bg-gray-800 rounded-xl border border-gray-700 shadow-lg ${extraClass}`}>
			{children}
		</div>
	);
}
