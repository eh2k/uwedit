//go build -x -ldflags=all='-H windowsgui -s -w' -o ./bin/uwedit.exe

package main

import (
	"fmt"
	//"io"
	"bufio"
	"encoding/json"
	"io/ioutil"
	"log"
	"math"
	"os"
	"path/filepath"
	"runtime"
	"time"

	"encoding/hex"

	"./core"
	"./icons"
	"github.com/eh2k/imgui-glfw-go-app"
	"github.com/eh2k/imgui-glfw-go-app/imgui-go"
	"github.com/eh2k/osdialog-go"
	"github.com/go-gl/glfw/v3.3/glfw"
)

func init() {
	// This is needed to arrange that main() runs on main thread.
	// See documentation for functions that are only allowed to be called from the main thread.
	runtime.LockOSThread()
}

type appSettings struct {
	File        string
	SilencePos  int32
	Silence     int32
	Delay       int32
	Midiout     int //DeviceIndex
	AudioDevice int
	TurboMidi   bool
}

func defaultSettings() appSettings {
	return appSettings{Midiout: core.ElektronDevice(), Delay: 50, AudioDevice: 0, Silence: 10}
}

var settings = defaultSettings()

type wavePanel struct {
	wave   []int16
	start  int32
	end    int32
	pos    int32
	cur    int32
	offset int32
	zoom   float32
	num    int8
	name   string
}

func (m wavePanel) Len() int32 {
	return int32(len(m.wave))
}

func (m wavePanel) ScreenXToWorld(x float32, screenDX float32) int32 {
	pos := int32((float32(m.offset) / m.zoom) + ((x / screenDX) * (float32(len(m.wave)) / m.zoom)))

	if pos < 0 {
		pos = 0
	}
	if pos >= m.Len() {
		pos = m.Len() - 1
	}

	return pos
}

func (m wavePanel) WorldXToScreen(wx int32, screenDX float32) float32 {
	return float32(((float32(wx) - (float32(m.offset) / m.zoom)) / (float32(len(m.wave)) / m.zoom)) * screenDX)
}

func (m wavePanel) Draw() {

	drawList := imgui.WindowDrawList()
	dx := imgui.WindowSize().X - 16
	dy := imgui.WindowSize().Y
	ox := imgui.WindowPos().X + 8
	oy := imgui.WindowPos().Y

	//m.pos = m.ScreenXToWorld(imgui.MousePos().X - 8, dx)

	last := float32(0)
	pos := m.ScreenXToWorld(0, dx)
	pos2 := m.ScreenXToWorld(dx-1, dx)

	if float32(pos2-pos) > dx {
		for x := float32(0); x < dx; x++ {
			i := m.ScreenXToWorld(x, dx)
			i2 := m.ScreenXToWorld(x+1, dx)

			v1 := m.wave[i]
			v2 := m.wave[i]

			for ; i <= i2; i++ {
				if v1 > m.wave[i] {
					v1 = m.wave[i]
				}

				if v2 < m.wave[i] {
					v2 = m.wave[i]
				}
			}

			vv1 := float32(v1) / float32(math.MaxInt16) * dy / 2 * 0.8
			vv2 := float32(v2) / float32(math.MaxInt16) * dy / 2 * 0.8

			drawList.AddLine(imgui.Vec2{X: ox + x, Y: oy + (dy / 2) + vv1}, imgui.Vec2{X: ox + x + 1, Y: oy + (dy / 2) + vv2}, imgui.PackedColor(0xff00ff00))
		}

	} else {
		for i := pos; i < pos2; i++ {

			v := float32(m.wave[i])
			v = (v / float32(math.MaxInt16) * (dy - oy) / 2 * 0.9) + (oy + dy/2)

			if i == pos {
				last = v
			}

			x1 := m.WorldXToScreen(i, dx)
			x2 := m.WorldXToScreen(i+1, dx)
			drawList.AddLine(imgui.Vec2{X: ox + x1, Y: last}, imgui.Vec2{X: ox + x2, Y: v}, imgui.PackedColor(0xff00ff00))
			last = v
		}
	}

	drawList.AddLine(imgui.Vec2{X: ox, Y: oy + dy/2}, imgui.Vec2{X: ox + dx, Y: oy + dy/2}, imgui.PackedColor(0x55ffffff))

	if context.num >= 25 {
		for i := int32(0); i < context.Len(); i += (context.Len() / 16) {
			x := context.WorldXToScreen(i, dx)
			drawList.AddLine(imgui.Vec2{X: ox + x, Y: oy}, imgui.Vec2{X: ox + x, Y: oy + dy}, imgui.PackedColor(0x22ffffff))
		}
	}

	cpos := m.WorldXToScreen(m.pos, dx)
	drawList.AddLine(imgui.Vec2{X: ox + cpos, Y: oy}, imgui.Vec2{X: ox + cpos, Y: oy + dy}, imgui.PackedColor(0xff0000ff))

	scur := m.WorldXToScreen(m.cur, dx)
	drawList.AddLine(imgui.Vec2{X: ox + scur, Y: oy}, imgui.Vec2{X: ox + scur, Y: oy + dy}, imgui.PackedColor(0xaaffffff))

	if m.start >= 0 && m.end >= 0 {
		spos := m.WorldXToScreen(m.start, dx)
		drawList.AddLine(imgui.Vec2{X: ox + spos, Y: oy}, imgui.Vec2{X: ox + spos, Y: oy + dy}, imgui.PackedColor(0xffff00ff))
		epos := m.WorldXToScreen(m.end, dx)
		drawList.AddLine(imgui.Vec2{X: ox + epos, Y: oy}, imgui.Vec2{X: ox + epos, Y: oy + dy}, imgui.PackedColor(0xffff00ff))
		drawList.AddRectFilled(imgui.Vec2{X: ox + spos, Y: oy}, imgui.Vec2{X: ox + epos, Y: oy + dy}, imgui.PackedColor(0x44ffffff))
		drawList.AddRectFilled(imgui.Vec2{X: ox + 0, Y: oy}, imgui.Vec2{X: ox + dx, Y: oy + dy}, imgui.PackedColor(0x11ffffff))
	}

	imgui.SetCursorPos(imgui.Vec2{X: 8, Y: dy - 16})
	imgui.Text(fmt.Sprintf("Sample-Length: %d, Loop-Start: %d, Loop-End: %d, Cursor: %d [%d]", context.Len(), context.start, context.end, context.pos, int32(float64(context.pos)/float64(context.Len())*128.0)))

}

func (m *wavePanel) OnMouseWheel(z float32) {

	if z < 0 {
		z = 0.9
	} else {
		z = 1.1
	}

	len := float32(len(m.wave))
	offScreen := ((len * m.zoom * z) - len) - ((len * m.zoom) - len)

	m.offset += int32(((float32(m.pos)) / len) * offScreen)

	m.zoom *= z

	if m.zoom < 1 {
		m.zoom = 1.0
		m.offset = 0
	}
}

func (m *wavePanel) OnMouseMove(w *glfw.Window, xx float64, yy float64) {

	w.SetCursor(nil)

	ddx, _ := w.GetSize()
	dx := float32(ddx - 16)
	x := float32(xx) - 8

	l := float32(len(m.wave))
	d := int32(l / 200.0 / m.zoom)

	startHitted := (m.pos < m.start+d) && (m.pos > m.start-d) && m.start != -1
	endHitted := (m.pos < m.end+d) && (m.pos > m.end-d) && m.end != -1

	if startHitted || endHitted {
		w.SetCursor(glfw.CreateStandardCursor(glfw.HResizeCursor))
	}

	if w.GetMouseButton(0) == 1 && !startHitted && !endHitted {
		//w.SetCursor(glfw.CreateStandardCursor(glfw.HResizeCursor))
		//m.offset += int32(((float32(m.pos) - x) / x) * l)
		//core.StartPlay(-1, context.wave, &context.cur, &context.start, &context.end)
	} else if w.GetMouseButton(1) == 1 {
		showContextMenu = true
	}

	if m.offset < 0 {
		m.offset = 0
	}

	maxOffset := (l * m.zoom) - l

	m.offset = int32(math.Min(float64(maxOffset), float64(m.offset)))

	m.pos = m.offset + int32(((x / dx) * l))
	m.pos = int32(float32(m.pos) / m.zoom)

	if m.pos < 0 {
		m.pos = 0
	}
	if m.pos > m.Len() {
		m.pos = m.Len()
	}

	if w.GetMouseButton(0) == 1 {
		if startHitted {
			m.SetLoop(m.pos, true)
		} else if endHitted {
			m.SetLoop(m.pos, false)
		}
	}
}

func (m *wavePanel) SetLoop(pos int32, start bool) {
	if pos < 0 {
		pos = 0
	}
	if pos > m.Len() {
		pos = m.Len()
	}

	if start {
		if pos > m.end {
			m.end = m.pos
		} else {
			m.start = pos
		}
	} else {
		if pos < m.start {
			m.start = pos
		} else {
			m.end = pos
		}
	}

	if m.start >= 0 && m.end < 0 {
		m.end = m.Len() - 1
	}

	if m.start < 0 && m.end >= 0 {
		m.start = 0
	}
}

var context = wavePanel{zoom: 1.0, start: -1, end: -1, name: "SNBD", num: 1}

var (
	loadProgress    float32 = 0
	showContextMenu         = false
)

func loop(displaySize imgui.Vec2) {

	var openFileDialog = false
	var saveFileDialog = false
	var showAboutWindow = false
	var showMidiSettings = false
	var insertSilence = false
	var sendSample = false

	io := imgui.CurrentIO()

	imgui.SetNextWindowPos(imgui.Vec2{X: 0, Y: 55.0})
	imgui.SetNextWindowSize(displaySize.Minus(imgui.Vec2{X: 0, Y: 70}))
	if imgui.BeginV("1", nil, imgui.WindowFlagsNoInputs|imgui.WindowFlagsNoDecoration|imgui.WindowFlagsNoSavedSettings|imgui.WindowFlagsNoBackground|imgui.WindowFlagsNoSavedSettings) {
		context.Draw()
		imgui.End()
	}

	imgui.SetNextWindowPos(imgui.Vec2{X: 0, Y: displaySize.Y - 40.0})
	imgui.SetNextWindowSize(imgui.Vec2{X: displaySize.X, Y: float32(40)})
	imgui.SetNextWindowContentSize(imgui.Vec2{X: (displaySize.X*context.zoom - 16), Y: float32(10)})
	if imgui.BeginV("hscrollbar", nil, imgui.WindowFlagsHorizontalScrollbar|imgui.WindowFlagsNoTitleBar|imgui.WindowFlagsNoResize|imgui.WindowFlagsNoBackground|imgui.WindowFlagsNoSavedSettings) {
		//imgui.PushStyleVarVec2(imgui.StyleVarItemSpacing, imgui.Vec2{X: 0, Y: 0})
		//defer imgui.PopStyleVar()
		if io.WantCaptureMouse() {
			sx := imgui.GetScrollX()
			context.offset = int32(sx / displaySize.X * float32(context.Len()))
		} else {
			sx := float32(context.offset) / float32(context.Len()) * displaySize.X
			imgui.SetScrollX(sx)
		}
		imgui.End()
	}

	if app.ImguiToolbarsBegin() {

		app.ImguiToolbar("File", 60, func() {

			if imgui.ImageButton(app.GetTexture(icons.FILEOPEN_PNG), imgui.Vec2{X: 16, Y: 16}) {
				openFileDialog = true
				core.StopPlay()
			}

			imgui.SameLine()

			if imgui.ImageButton(app.GetTexture(icons.FILESAVE_PNG), imgui.Vec2{X: 16, Y: 16}) {
				saveFileDialog = true
				core.StopPlay()
			}
		})

		app.ImguiToolbar("Play", 60, func() {

			if imgui.ImageButton(app.GetTexture(icons.MEDIA_PLAYBACK_START_PNG), imgui.Vec2{X: 16, Y: 16}) {
				context.cur = 0
				core.StartPlay(settings.AudioDevice, context.wave, &context.cur, &context.start, &context.end)
			}

			imgui.SameLine()

			if imgui.ImageButton(app.GetTexture(icons.MEDIA_PLAYBACK_STOP_PNG), imgui.Vec2{X: 16, Y: 16}) {
				core.StopPlay()
			}
		})

		app.ImguiToolbar("Name", 60, func() {

			imgui.InputTextV("", &context.name, imgui.InputTextFlagsCallbackAlways, func(data imgui.InputTextCallbackData) int32 {

				if len(context.name) > 4 {
					data.DeleteBytes(4, 1)
				}

				return 0
			})
		})

		app.ImguiToolbar("Position", 60, func() {

			if imgui.BeginCombo(" ", fmt.Sprint(context.num)) {

				for i := int8(1); i <= 48; i++ {
					entry := fmt.Sprint(i)

					if imgui.SelectableV(entry, context.num == i, 0, imgui.Vec2{X: 30, Y: 14}) {
						context.num = i
					}

					if context.num == i && imgui.IsWindowAppearing() {
						imgui.SetScrollY(imgui.CursorPosY() - imgui.WindowHeight()/2)
					}
				}
				imgui.EndCombo()
			}
		})

		app.ImguiToolbar("MidiOut", float32(20+(len(core.GetMidiDevices()[settings.Midiout])*8)), func() {

			if imgui.BeginCombo("  ", core.GetMidiDevices()[settings.Midiout]) {
				// devices := core.GetMidiDevices()
				// for i := 0; i < len(devices); i++ {
				// 	entry := devices[i]
				// 	if imgui.Selectable(entry) {
				// 		context.Midiout = i
				// 	}
				// }
				imgui.EndCombo()
				showMidiSettings = true
			}
		})

		app.ImguiToolbar("Send", 50, func() {

			if imgui.ImageButton(app.GetTexture(icons.MD_PNG), imgui.Vec2{X: 30, Y: 16}) {
				sendSample = true
			}

			if imgui.IsItemHovered() {
				imgui.SetTooltip("Upload to Machinedrum..")
			}

			imgui.SetCursorPos(imgui.Vec2{X: 56, Y: 25})

			if false && imgui.Checkbox("Turbo", &settings.TurboMidi) {

				//https://www.elektron.se/wp-content/uploads/2016/05/machinedrum_manual_OS1.63.pdf

				fmt.Println("Turbo", settings.TurboMidi)
				core.MidiOutOpenPort(settings.Midiout)
				defer core.MidiOutClosePort()

				core.MidiInOpenPort(settings.Midiout)
				defer core.MidiInClosePort()

				header := []byte{0xF0, 0x00, 0x20, 0x3c, 0x00, 0x00}
				if settings.TurboMidi {

					done := make(chan bool, 1)

					core.MidiOutSendMessage(append(header, []byte{0x10, 0xF7}...))

					core.MidiInSetCallback(func(time float64, bytes []byte) {
						fmt.Println("MidiIn:", time, hex.Dump(bytes))
						done <- true
					})

					<-done

					core.MidiOutSendMessage(append(header, []byte{0x12, 0x0f, 0x01, 0xF7}...))

				} else {
					turboOn := []byte{0xf0, 0x00, 0x20, 0x3c, 0x00, 0x00, 0x12, 0x01, 0x01, 0xf7}
					core.MidiOutSendMessage(turboOn)
				}
			}

		})

		app.ImguiToolbar("About", 50, func() {

			if imgui.ImageButton(app.GetTexture(icons.INFO_PNG), imgui.Vec2{X: 30, Y: 16}) {
				showAboutWindow = true
			}
		})

		app.ImguiToolbarsEnd()
	}

	if showMidiSettings {
		showMidiSettings = false
		imgui.OpenPopup("Midi-Settings")
	}

	imgui.SetNextWindowSize(imgui.Vec2{X: float32(400), Y: float32(250)})
	if imgui.BeginPopupModalV("Midi-Settings", nil, imgui.WindowFlagsNoResize|imgui.WindowFlagsNoSavedSettings) {

		imgui.Text("MidiOut:")

		if imgui.BeginCombo("   ", core.GetMidiDevices()[settings.Midiout]) {
			devices := core.GetMidiDevices()
			for i := 0; i < len(devices); i++ {
				entry := devices[i]
				if imgui.Selectable(entry) {
					settings.Midiout = i
				}
			}

			imgui.EndCombo()
		}

		// devices := core.GetMidiDevices()
		// for i := 0; i < len(devices); i++ {
		// 	entry := devices[i]
		// 	if imgui.RadioButton(entry, settings.Midiout == i) {
		// 		settings.Midiout = i
		// 	}
		// }

		imgui.Spacing()

		imgui.Text("Delay (ms):")

		if imgui.InputIntV("  ", &settings.Delay, 10, 50, 0) && settings.Delay < 0 {
			settings.Delay = 0
		}

		imgui.Spacing()
		imgui.Separator()
		imgui.Spacing()

		imgui.Text("AudioOut:")
		if imgui.BeginCombo("    ", core.GetAudioDevices()[settings.AudioDevice]) {

			devices := core.GetAudioDevices()
			for i := 0; i < len(devices); i++ {
				entry := devices[i]
				if imgui.Selectable(entry) {
					settings.AudioDevice = i
				}
			}

			imgui.EndCombo()
		}

		imgui.Spacing()

		imgui.SetCursorPos(imgui.WindowSize().Minus(imgui.Vec2{X: float32(40), Y: float32(38)}))
		imgui.Separator()
		imgui.SetCursorPos(imgui.WindowSize().Minus(imgui.Vec2{X: float32(40), Y: float32(28)}))
		if imgui.Button("OK") {
			imgui.CloseCurrentPopup()
		}

		imgui.EndPopup()
	}

	if showContextMenu {
		showContextMenu = false
		imgui.OpenPopup("ContextMenu")
	}

	if imgui.BeginPopup("ContextMenu") {

		if imgui.Selectable("Insert Silence..") {
			insertSilence = true
		}

		const silenceLevel = int16(100)
		if imgui.Selectable("Remove Silence from beginning") {
			i := 0
			for ; i < len(context.wave); i++ {
				if context.wave[i] > silenceLevel || context.wave[i] < -silenceLevel {
					break
				}
			}
			context.wave = context.wave[i:]
		}

		if imgui.Selectable("Remove Silence from end") {
			i := len(context.wave) - 1
			for ; i > 0; i-- {
				if context.wave[i] > silenceLevel || context.wave[i] < -silenceLevel {
					break
				}
			}
			context.wave = context.wave[:i+1]
		}

		imgui.Separator()

		if imgui.Selectable("Invert waveform") {

			for i := 0; i < len(context.wave); i++ {
				context.wave[i] *= -1
			}
		}

		if imgui.Selectable("Normalize") {

			max := int16(0)
			for i := 0; i < len(context.wave); i++ {

				if context.wave[i] > 0 {
					if max < context.wave[i] {
						max = context.wave[i]
					}
				} else {
					if max < context.wave[i]*-1 {
						max = context.wave[i] * -1
					}
				}
			}

			f := float32(math.MaxInt16) / float32(max)
			for i := 0; i < len(context.wave); i++ {
				context.wave[i] = int16(float32(context.wave[i]) * f)
			}
		}

		imgui.Separator()

		if imgui.Selectable("Set Loop-Start at cursor") {
			context.SetLoop(context.pos, true)
		}
		if imgui.Selectable("Set Loop-End at cursor") {
			context.SetLoop(context.pos, false)
		}
		if imgui.Selectable("Loop entire sample") {
			context.SetLoop(0, true)
			context.SetLoop(context.Len()-1, false)
		}
		if imgui.Selectable("Remove Loop-Points") {
			context.end = -1
			context.start = -1
		}

		imgui.EndPopup()
	}

	if insertSilence {
		imgui.OpenPopup("Insert-Silence")
	}

	//imgui.SetNextWindowSize(imgui.Vec2{X: float32(400), Y: float32(250)})
	if imgui.BeginPopupModalV("Insert-Silence", nil, imgui.WindowFlagsNoResize|imgui.WindowFlagsNoSavedSettings) {

		imgui.Text("Position:")
		if imgui.RadioButton("Beginning", settings.SilencePos == 0) {
			settings.SilencePos = 0
		}
		imgui.SameLine()
		if imgui.RadioButton("End", settings.SilencePos == 1) {
			settings.SilencePos = 1
		}
		imgui.Text("")
		imgui.Text("Silence Length (ms):            ")

		if imgui.InputIntV("  ", &settings.Silence, 10, 10, 0) && settings.Silence < 0 {
			settings.Silence = 0
		}

		imgui.Text("")
		imgui.Text("")
		imgui.Text("")
		imgui.SetCursorPos(imgui.WindowSize().Minus(imgui.Vec2{X: float32(0), Y: float32(38)}))
		imgui.Separator()
		imgui.SetCursorPos(imgui.WindowSize().Minus(imgui.Vec2{X: float32(90), Y: float32(28)}))
		if imgui.Button("OK") {
			silence := make([]int16, settings.Silence*48)
			switch settings.SilencePos {
			case 0:
				context.wave = append(silence, context.wave...)
				context.start += int32(len(silence))
				context.end += int32(len(silence))
			case 1:
				context.wave = append(context.wave, silence...)
			case 2:
				context.wave = append(context.wave[:context.pos], append(silence, context.wave[context.pos:]...)...)
				context.end = -1
				context.start = -1
			}
			imgui.CloseCurrentPopup()
		}
		imgui.SameLine()
		if imgui.Button("Cancel") {
			imgui.CloseCurrentPopup()
		}
		imgui.EndPopup()
	}

	app.ShowAboutPopup(&showAboutWindow, "uwedit", "0.9.10", "Copyright (C) 2010-2020 by E.Heidt", "https://github.com/eh2k/uwedit")

	if sendSample {
		sendSample = false
		imgui.OpenPopup("Upload")
		loadProgress = 0
		go func() {

			loadProgress = 0
			time.Sleep(0)
			// for j := 0; j < 10; j++ {
			// 	time.Sleep(50000000)
			// 	loadProgress += 0.1
			// 	fmt.Println(loadProgress)
			// }
			core.SendToMachineDrum(settings.Midiout, settings.Delay,
				core.Sample{Wave: context.wave, Name: context.name, Num: int32(context.num), LoopStart: context.start, LoopEnd: context.end},
				func(p float32) {
					loadProgress = p
				})
			loadProgress = 1
		}()
	}

	//imgui.SetNextWindowPos(imgui.Vec2{X: float32(width)/2 - 150.0, Y: float32(height)/2 - 20.0})
	if imgui.BeginPopupModalV("Upload", nil, imgui.WindowFlagsNoResize|imgui.WindowFlagsNoSavedSettings|imgui.WindowFlagsNoTitleBar) {
		imgui.Text("uploading...")
		imgui.ProgressBarV(loadProgress, imgui.Vec2{X: 300, Y: 22}, "")
		if loadProgress >= 1 {
			imgui.CloseCurrentPopup()
		}

		imgui.EndPopup()
	}

	if openFileDialog {
		openFileDialog = false
		filename, err := osdialog.ShowOpenFileDialog(".", context.name+".wav", "Sample Files:wav,syx;Wave file (*.wav):wav;Midi Sample Dumps (*.syx):syx")
		if err == nil {
			loadSample(filename)
		}
	}

	if saveFileDialog {
		saveFileDialog = false
		filename, err := osdialog.ShowSaveFileDialog(".", context.name+".wav", "Wave file (*.wav):wav;Midi Sample Dumps (*.syx):syx")
		if err == nil {
			core.SaveSample(filename,
				core.Sample{Wave: context.wave, Name: context.name, Num: int32(context.num), LoopStart: context.start, LoopEnd: context.end})
		}
	}
}

var (
	setWindowTitle = func(title string) {}
)

func loadSample(filename string) {

	core.StopPlay()

	s := core.LoadSample(filename)
	settings.File = filename
	context.wave = s.Wave
	context.name = s.Name

	if s.Num > 0 {
		context.num = int8(s.Num)
	}

	context.cur = 0
	context.start = s.LoopStart
	context.end = s.LoopEnd

	setWindowTitle("UWedit - " + filename)
}

func main() {

	log.SetFlags(log.LstdFlags | log.Lshortfile)

	exePath, err := os.Executable()
	if err != nil {
		log.Println(err)
	}

	err3 := os.Chdir(filepath.Dir(exePath))
	if err3 != nil {
		log.Println(err3)
	}

	settingsFile := filepath.Join(filepath.Dir(exePath), ".uwedit.json")
	{
		settingsJSON, err := ioutil.ReadFile(settingsFile)
		if err == nil && settingsJSON != nil {
			err = json.Unmarshal(settingsJSON, &settings)
			if err != nil {
				settings = defaultSettings()
			}
		}

		fmt.Println("settings:", settings)
	}

	defer func() {
		if settings != defaultSettings() {
			settingsJSON, _ := json.MarshalIndent(settings, "", " ")
			_ = ioutil.WriteFile(settingsFile, settingsJSON, 0644)
		}
	}()

	// core.MidiIn_setCallback(func(time float64, bytes []byte) {
	// 	fmt.Println("MidiIn:", time, hex.Dump(bytes))
	// })

	// core.MidiIn_openPort(settings.Midiout)
	// defer core.MidiIn_closePort()

	window := app.NewAppWindow(640, 400)

	setWindowTitle = func(title string) {
		window.SetTitle(title)
	}

	defer app.Dispose()

	stat, err := os.Stdin.Stat()
	if len(os.Args) > 1 && err == nil && (stat.Mode()&os.ModeCharDevice == 0) {
		data, _ := ioutil.ReadAll(os.Stdin)
		tmpFile := os.TempDir() + "/temp.wav"
		if len(os.Args) > 1 {
			tmpFile = os.Args[1]
			fmt.Println(tmpFile)
		}
		file, err := os.Create(tmpFile)
		if err != nil {
			log.Fatal(err)
		}
		writer := bufio.NewWriter(file)
		_, err = writer.Write(data)
		if err != nil {
			log.Fatal(err)
		}
		loadSample(tmpFile)
		os.Remove(tmpFile)

	} else if len(os.Args) > 1 {
		loadSample(os.Args[1])
	} else {
		fmt.Println("asdf")
		if _, err := os.Stat(settings.File); !os.IsNotExist(err) {
			loadSample(settings.File)
		} else {
			n := 512.0
			lastN := 0.0

			for f := 2.0; f < n; f = f * 1.5 {
				lastN = f
				i := 0.0
				j := 0.0
				k := 1.0
				for ; i < f; i++ {

					v := int16(math.Sin((math.Pi/f)*j*2) * 0x5FFF)
					context.wave = append(context.wave, v)
					j += k
				}
			}

			context.start = context.Len() - int32(lastN)
			context.end = context.Len() - 1
		}
	}

	{
		io := imgui.CurrentIO()

		log.Println("OK")

		window.SetScrollCallback(func(w *glfw.Window, xoff float64, yoff float64) {
			if io.WantCaptureMouse() {
				io.AddMouseWheelDelta(float32(xoff), float32(yoff))
			} else {
				context.OnMouseWheel(float32(yoff))
			}
		})

		window.SetMouseButtonCallback(func(w *glfw.Window, button glfw.MouseButton, action glfw.Action, mods glfw.ModifierKey) {

			if io.WantCaptureMouse() {
				io.SetMouseButtonDown(int(button), action == 1)
			} else {
				x, y := w.GetCursorPos()
				context.OnMouseMove(w, x, y)
			}
		})

		window.SetCursorPosCallback(func(w *glfw.Window, x float64, y float64) {

			if w.GetAttrib(glfw.Focused) != 0 {
				io.SetMousePosition(imgui.Vec2{X: float32(x), Y: float32(y)})
			} else {
				io.SetMousePosition(imgui.Vec2{X: -math.MaxFloat32, Y: -math.MaxFloat32})
			}

			if !io.WantCaptureMouse() {
				context.OnMouseMove(w, x, y)
			}
		})

		io.KeyMap(imgui.KeyBackspace, int(glfw.KeyBackspace))

		window.SetCharCallback(func(window *glfw.Window, char rune) {
			io.AddInputCharacters(string(char))
		})

		window.SetKeyCallback(func(window *glfw.Window, key glfw.Key, scancode int, action glfw.Action, mods glfw.ModifierKey) {
			if action == glfw.Press {
				io.KeyPress(int(key))
			}
			if action == glfw.Release {
				io.KeyRelease(int(key))
			}

			if action == glfw.Press && !io.WantCaptureMouse() {

				if key == glfw.KeySpace {
					context.cur = 0
					core.StartPlay(settings.AudioDevice, context.wave, &context.cur, &context.start, &context.end)
				} else if key == glfw.KeyLeft || key == glfw.KeyUp {

					files, err := ioutil.ReadDir(filepath.Dir(settings.File))
					if err == nil {
						for i, f := range files {
							if f.Name() == filepath.Base(settings.File) && i > 0 {
								loadSample(filepath.Join(filepath.Dir(settings.File), files[i-1].Name()))
								return
							}
						}
					}
				} else if key == glfw.KeyRight || key == glfw.KeyDown {

					files, err := ioutil.ReadDir(filepath.Dir(settings.File))
					if err == nil {
						for i, f := range files {
							if f.Name() == filepath.Base(settings.File) && i < len(files)-1 {
								loadSample(filepath.Join(filepath.Dir(settings.File), files[i+1].Name()))
								return
							}
						}
					}
				}
			}

			io.KeyCtrl(int(glfw.KeyLeftControl), int(glfw.KeyRightControl))
			io.KeyShift(int(glfw.KeyLeftShift), int(glfw.KeyRightShift))
			io.KeyAlt(int(glfw.KeyLeftAlt), int(glfw.KeyRightAlt))
			io.KeySuper(int(glfw.KeyLeftSuper), int(glfw.KeyRightSuper))
		})
	}

	app.Run(loop)

	fmt.Println("END")
}
