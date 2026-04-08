import './style.css';

// Example for a component as a function which is stateless
export function Resource(props) {
	return (
		<a href={props.href} target="_blank" class="resource" rel="noreferrer">
			<h2>{props.title}</h2>
			<p>{props.description}</p>
		</a>
	);
}
