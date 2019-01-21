#include <iostream>

#include "olcNoiseMaker.h"

using namespace std;

atomic<double> dFrequencyOutput = 0.0;

double MakeNoise(double dRunTime) {
	double dOutput = sin(2 * PI * dFrequencyOutput * dRunTime);

	if (dOutput > 0.0)
		return 0.1;

	return -0.1;
}

int main() {
	vector<wstring> devices = olcNoiseMaker<short>::Enumerate(); // Get sound hardware

	for (auto device : devices) wcout << "Found output device: " << device << endl;

	// Use the first sound hardware, which is usually the default sound device
	// The data type represents, like, the amount of values in bits that our sound wave can be represented as, for example
	// 'short' represents 16bits or 'char' would be 8bits.
	olcNoiseMaker<short> sound(devices.at(0), 44100, 1, 8, 512);

	// Link the noise making function to the sound machine
	sound.SetUserFunction(MakeNoise);

	// If we take 440Hz and halve it twice, we get 110Hz which means that we went down 2 octaves
	// Conventionally, there are 12 notes per octave
	double dOctaveBaseFrequency = 110.0; // A2
	double d12thRootOf2 = pow(2.0, 1.0 / 12.0);

	// Since GetAsync returns a 16 bit short value, we mask out the most significant bit that represents if the key is pressed or not
	while (true) {
		bool isKeyPressed{ false };
		
		for (int i = 0; i < 15; i++) {
			if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe"[i])) & 0x8000) {
				dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, i);
				isKeyPressed = true;
			}
		}

		if (!isKeyPressed)
			dFrequencyOutput = 0.0;
	}

	return 0;
}