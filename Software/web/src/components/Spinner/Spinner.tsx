import './style.css';
import { ComponentChildren } from 'preact';

type SpinnerProps = {
	loading: boolean;
	children: ComponentChildren;
};

// Example for a component as a function which is stateless
export function Spinner({ loading, children }: SpinnerProps) {
	return (
		<div class="spinner-container">
			{loading && (
				<div class='overlay'>
					<div class="spinner" />
				</div>
			)}
			{children}
		</div>

	);
}
