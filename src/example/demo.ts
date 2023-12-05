import { EventEmitter } from 'events';
import { resolve, dirname } from "path";
import { fileURLToPath } from "url";
import { inherits } from 'util';
import bindings from "bindings";

// import { NodeTray } from "../build/Release/tray";
const { NodeTray } = bindings("tray");

const __dirname = dirname(fileURLToPath(import.meta.url));

inherits(NodeTray, EventEmitter);

// process.title = 'Tray Demo';
const trayTitle = "Tray Demo";

const tray = new NodeTray(resolve(__dirname, "../../resource/image/compass-orig.ico"));
tray.setToolTip(trayTitle);

tray.on('click', () => {

	let result = tray.toggleWindow(trayTitle);
	console.log("click, result = ", result);

})
tray.on('right-click', () => {

	// console.log("right-click");

	var menu = [
		{
			id: 10,
			title: 'Item 1',
		},
		{
			id: 20,
			title: 'Item 2',
		},
		{
			id: 30,
			title: '---',
		},
		{
			id: 50,
			title: 'Exit',
		},
	];

	tray.showPopup(menu, function (err: Error, result: any) {

		console.log('error:', err);
		console.log('result:', result);

		if (result == 50)
		{
			shutdown();
			return;
		}

	});

})
tray.on('balloon-click', () => {

	console.log('balloon-click')

})

setInterval(function () {

	// console.log('Window Visibility: ', tray.isWindowVisible(trayTitle));
	// console.log('Window Minimized: ', tray.isWindowMinimized(trayTitle));

	if (tray.isWindowMinimized(trayTitle) == true)
	{
		tray.toggleWindow(trayTitle);

		tray.balloon(trayTitle, 'Will continue in background', 5000);
		return;
	}

	console.log('In background');

}, 1000)

// tray.on("double-click", () => {
// 	tray.destroy()
// 	process.exit(0)
// })

var SHUTDOWN_EVENTS = [
	'exit',
	'SIGINT',
	'SIGUSR1',
	'SIGUSR2',
	'SIGTERM',
	'uncaughtException',
];

var onShutdown = function (cb: () => void) {
	for (let i = 0; i < SHUTDOWN_EVENTS.length; i++)
	{
		let event = SHUTDOWN_EVENTS[i];
		process.on(event, cb);
	}
};

var shutdown = function(): void {
	console.log('Shutdown!');
	tray.destroy();
	process.exit(0);
}

onShutdown(shutdown);
