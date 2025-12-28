const { app, BrowserWindow } = require('electron')
const { pathToFileURL } = require('url')

const koffi = require('koffi')

const cablegen = koffi.load('/home/emelia/programming/cablegen/cablegen.so')

const cablegen_test = cablegen.func('bool test(void)')

cablegen_test();

const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  })
  win.loadURL(pathToFileURL('./index.html').href)
}

app.whenReady().then(() => {
  createWindow()
  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})
