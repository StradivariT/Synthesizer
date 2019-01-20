#include <iostream>

#include "olcNoiseMaker.h"

using namespace std;

double dFrequencyOutput = 0.0;

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

	while (true) {
		if (GetAsyncKeyState('A'))
			dFrequencyOutput = 440.0;
		else
			dFrequencyOutput = 0.0;
	}

	return 0;
}