echo "LA for 1 sec"
synth 1000 sin 440

echo "220Hz chopped sinus 5x/sec"
synth 1000 am 0 100 sinus 220 square 5

echo "AM on sinus 440Hz"
synth 1000 am 0 100 sinus 220 sinus 5

echo "Mixing two signals, triangle 440Hz 50% volume and sinus 330Hz for 1sec"
synth 1000 tri 440:50 sinus 330

echo "... Hum how to define that ??? (brackets here for readability)"
synth 10000 { am 0 100 { fm 80 120 sq 440:25 tri 1 } sq 5 } { am 0 100 { fm 80 120 sq 330:25 tri 1 } square 6 } { fm 80 120 sinus 1200:30 sinus 3 }

echo "Using generator/modulator/both flag for fm generator : "
echo "The 1st fm speed modulation has an effect on the 2nd fm :"
echo "1st effect, modify slowly frequency"
synth fm 60 140 { fm 60 140 generator sinus 800 sinus 10 } sinus 1
echo "2nd effect, modify the modulation frequency"
synth fm 60 140 { fm 60 140 modulator sinus 800 sinus 10 } sinus 1
echo "3rd effect, modify both"
synth fm 60 140 { fm 60 140 both sinus 800 sinus 10 } sinus 1
echo "Is it an ufo ?"
synth envelope 10000 once data 0 100 75 50 25 0 end fm 60 140 { fm 60 140 modulator square 880 sinus 50 } sinus 10

echo "White noise"
synth 2000 wnoise

echo "Trying to envelope the white noise to create sound of waves, am is here to make sound more crackly"
synth 2000 am 100 100 envelope 2000 once data 50 100 20 10 0 end wnoise sinus 1000
echo "Grouping together two generator (to be used in a single fm or am modulator for example)"
synth 2000 { sinus 440 sinus 880 }

echo "Playing on right channel only"
synth 2000 right { sinus 440 sinus 880 }

echo "Playing on left channel only"
synth 2000 left { sinus 440 sinus 880 }

echo "Playing on both different sounds with different volumes."
synth 2000 left sinus 440 right triangle 440:50

echo "Organ sound like with adsr generator"
synth adsr 1:100 200:50 400:50 450:0 500:0 loop { sinus 440 sinus 880:50 sinus 1320:25 }

echo "Using adsr generator to make beep beep sounds."
synth adsr 1:100 50:100 51:0 100:0 101:100 150:100 151:0 1000:0 loop triangle 440

echo "Reverb effect on bip bip"
synth 20000 reverb 1387:60 adsr 1:100 50:100 51:0 100:0 101:100 150:100 151:0 1000:0 loop triangle 440:90 

echo "Another reverb effect"
synth 10000 reverb 450:90 reverb 10:80 adsr 1:80 100:0 333:0 loop sinus 440:80

echo "This was a great sound effect on ZX Spectrum(tm)"
synth square 220 sq 222

echo "Is this an engine ?"
synth 30000 fm 100 150 am 0 100 square 100:20 square 39 adsr 1:0 1000:0 2000:100 5001:100 6000:-100 8000:0 loop level 1
echo "Another one"
synth 30000 reverb 10:50 fm 0 150 am 0 100 triangle 100:50 square 39 adsr 1:0 1000:0 2000:100 5001:400 6000:400 8000:-100 9000:0 loop level 1
synth 30000 reverb 10:50 fm 0 100 am 0 100 wnoise triangle 39 adsr 1:0 1000:0 2000:100 5001:400 6000:400 8000:-100 9000:0 loop level 1
synth 30000 reverb 10:50 fm 0 100 am 0 100 triangle 39 wnoise adsr 1:0 1000:0 2000:100 5001:400 6000:400 8000:0 9000:0 loop level 1
synth 30000 reverb 30:30 fm 0 100 am 0 100 square 39:30 triangle 100 adsr 1:0 1000:0 2000:100 3001:400 6000:400 8000:0 9000:0 loop level 1 reverb 30:50 { right am 0 100 sinus 440:35 sinus 1 left am 100 0 sinus 440:34 sinus 0.5 }
echo "Siren 1"
synth fm 50 100 sq 440:10 triangle 1 asc
echo "Siren 2"
synth fm 0 100 reverb 100:30 fm 50 100 sin 880:30 triangle 6  sinus 1
echo "Siren 3"
synth reverb 100:30 fm 50 100 sq 880:30 tri 5 asc
echo "Siren 4"
synth reverb 100:50 fm 90 100 sinus 1440:50 sinus 1.5
echo "How to modify sound in real time with the mouse for example"
mousynth 20000 -b 16 reverb 300:60 fm 0 200 sq 220:20 mouse_hook 0 5
