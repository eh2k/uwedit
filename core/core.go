package core

// #cgo CFLAGS: -I../dep/stk/include
// #cgo CXXFLAGS: -std=c++17 -I../dep/stk/include
// #cgo windows LDFLAGS: -static -static-libgcc -static-libstdc++ -L../dep/stk/src/ -lstk -lole32 -ldsound -lwinmm
// #cgo linux LDFLAGS: -ldl -lstdc++ -L../dep/stk/src/ -lstk -lasound
// #include <stdlib.h>
// #include <core.h>
import "C"
import (
	"fmt"
	"log"
	"path/filepath"
	"strings"
	"unsafe"
)

type Sample struct {
	Wave      []int16
	LoopStart int32
	LoopEnd   int32
	Num       int32
	Name      string
}

func LoadSample(path string) Sample {

	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))

	if strings.HasSuffix(path, ".syx") {
		r := make([]int16, 1024*1024)
		p := unsafe.Pointer(&r[0])

		tmp := Sample{Name: "asdf"}
		s := unsafe.Pointer(&tmp.LoopStart)
		e := unsafe.Pointer(&tmp.LoopEnd)
		n := make([]byte, 16)
		pn := unsafe.Pointer(&n[0])
		u := unsafe.Pointer(&tmp.Num)
		size := C.LoadMidiSDS(cs, (*C.short)(p), (*C.uint)(s), (*C.uint)(e), (*C.char)(pn), (*C.int)(u))
		tmp.Wave = r[:size]
		tmp.Name = string(n)
		tmp.Num++
		return tmp
	} else {
		size := C.LoadWaveFile(cs, nil, C.size_t(0))
		r := make([]int16, size)
		p := unsafe.Pointer(&r[0])
		C.LoadWaveFile(cs, (*C.short)(p), C.size_t(len(r)))
		return Sample{Wave: r, Name: strings.ToUpper(filepath.Base(path + "    ")[:4]), LoopStart: -1, LoopEnd: -1, Num: 48}
	}
}

func SaveSample(path string, sample Sample) {

	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	p := unsafe.Pointer(&sample.Wave[0])

	if strings.HasSuffix(path, ".syx") {

		cname := C.CString(sample.Name)
		defer C.free(unsafe.Pointer(cname))

		err := C.SaveMidiSDS(cs, (*C.short)(p), C.size_t(len(sample.Wave)), C.uint(sample.LoopStart), C.uint(sample.LoopEnd), cname, C.int(sample.Num-1))
		if err != 0 {
			log.Fatal(err)
		}

	} else {
		if !strings.HasSuffix(path, ".wav") {
			path += ".wav"
		}
		cs := C.CString(path)
		C.SaveWaveFile(cs, (*C.short)(p), C.size_t(len(sample.Wave)))
	}
}

var (
	audio_devices = []string{}
)

func GetAudioDevices() []string {

	if len(audio_devices) > 0 {
		return audio_devices
	}

	for i := 0; ; i++ {
		r := C.GetAudioDevice(C.int(i))
		if r == nil {
			break
		}
		gostr := C.GoString(r)
		audio_devices = append(audio_devices, gostr)
	}

	if len(midi_devices) == 0 {
		audio_devices = append(audio_devices, "no devices found")
	}

	return audio_devices
}

func StartPlay(deviceId int, wave []int16, play_pos *int32, loop_start *int32, loop_end *int32) {

	p := unsafe.Pointer(&wave[0])
	c := unsafe.Pointer(play_pos)
	s := unsafe.Pointer(loop_start)
	e := unsafe.Pointer(loop_end)
	C.Player_Play(C.int(deviceId), (*C.short)(p), C.size_t(len(wave)), (*C.int)(c), (*C.int)(s), (*C.int)(e))
}

func StopPlay() {
	C.Player_Stop()
}

var (
	midi_devices = []string{}
)

func ElektronDevice() int {

	for i, e := range GetMidiDevices() {
		if strings.HasPrefix(e, "Elektron") {
			return i
		}
	}

	return 0
}

func GetMidiDevices() []string {

	if len(midi_devices) > 0 {
		return midi_devices
	}

	for i := 0; ; i++ {
		r := C.GetMidiOutDevice(C.int(i))
		if r == nil {
			break
		}
		gostr := C.GoString(r)
		tmp := strings.Split(gostr, ":")
		if len(tmp) > 1 && strings.Contains(tmp[1], tmp[0]) {
			gostr = tmp[0] //strings.Join(tmp[1:], ":")
		}
		midi_devices = append(midi_devices, gostr)
	}

	if len(midi_devices) == 0 {
		midi_devices = append(midi_devices, "no devices found")
	}

	return midi_devices
}

var progressCB func(p float32) = func(p float32) {
	fmt.Println("progressCB", p)
}

//export goProgressCB
func goProgressCB(p float32) {
	progressCB(p)
}

func SendToMachineDrum(midiport int, deley int32, sample Sample, progress func(float32)) {

	progressCB = progress

	MidiOut_openPort(midiport)

	cs := C.CString(sample.Name)
	defer C.free(unsafe.Pointer(cs))

	p := unsafe.Pointer(&sample.Wave[0])
	C.SendMidiSDS(C.int(deley), (*C.short)(p), C.size_t(len(sample.Wave)),
		C.uint(sample.LoopStart), C.uint(sample.LoopEnd),
		cs,
		C.int(sample.Num-1))

	MidiOut_closePort()
}

func MidiOut_openPort(midiport int) {
	C.MidiOut_openPort(C.int(midiport))
}

func MidiOut_closePort() {
	C.MidiOut_closePort()
}

func MidiOut_sendMessage(data []byte) {

	p := unsafe.Pointer(&data[0])
	C.MidiOut_sendMessage((*C.uchar)(p), C.size_t(len(data)))
}

func MidiIn_openPort(midiport int) {
	fmt.Println("MidiIn_openPort", midiport)
	C.MidiIn_openPort(C.int(midiport))
}

func MidiIn_closePort() {
	fmt.Println("MidiIn_closePort")
	C.MidiIn_closePort()
}

var (
	_goMidiInCallback = func(p float64, data unsafe.Pointer, size int) {}
)

//export goMidiInCallback
func goMidiInCallback(p float64, data unsafe.Pointer, size int) {
	_goMidiInCallback(p, data, size)
}

func MidiIn_setCallback(cb func(time float64, data []byte)) {

	_goMidiInCallback = func(time float64, data unsafe.Pointer, size int) {
		bytes := C.GoBytes(data, C.int(size))
		cb(time, bytes)
	}

	C.MidiIn_setCallback()
}
