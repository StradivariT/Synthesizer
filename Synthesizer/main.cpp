#include <iostream>
#include "olcNoiseMaker.h"

using namespace std;

struct EnvelopeASDR {
	double attackTime;
	double decayTime;
	double releaseTime;

	double sustainAmplitude;
	double startAmplitude;

	double triggerOnTime;
	double triggerOffTime;

	bool isNotePressed;

	EnvelopeASDR() {
		attackTime = 0.10;
		decayTime = 0.1;
		startAmplitude = 1.0;
		sustainAmplitude = 0.8;
		releaseTime = 0.20;
		triggerOnTime = 0.0;
		triggerOffTime = 0.0;
		isNotePressed = false;
	}

	double GetAmplitude(double runTime) {
		double amplitude{ 0.0 };
		double lifeTime{ runTime - triggerOnTime };

		if (isNotePressed) {
			if (lifeTime <= attackTime)
				amplitude = (lifeTime / attackTime) * startAmplitude;

			if (lifeTime > attackTime && lifeTime <= attackTime + decayTime)
				amplitude = ((lifeTime - attackTime) / decayTime) * (sustainAmplitude - startAmplitude) + startAmplitude;

			if (lifeTime > attackTime + decayTime)
				amplitude = sustainAmplitude;
		} else {
			amplitude = ((runTime - triggerOffTime) / releaseTime) * (0.0 - sustainAmplitude) + sustainAmplitude;
		}

		return amplitude <= 0.0001 ? 0.0 : amplitude;
	}

	void NotePressed(double time) {
		triggerOnTime = time;
		isNotePressed = true;
	}

	void NoteReleased(double time) {
		triggerOffTime = time;
		isNotePressed = false;
	}
};

atomic<double> frequencyOutput{ 0.0 };
double masterVolume{ 0.4 };
enum waveTypes {
	Sine,
	Square,
	AnalogSquare,
	Triangle,
	AnalogSawtooth,
	OptimalSawtooth,
	Noise
};

struct Instrument {
	double volume;
	EnvelopeASDR envelope;

	virtual double GetSound(double runTime, double hz) = 0;
};

double Oscillator(double, double, waveTypes, double = 0.0, double = 0.0);

struct Bell : public Instrument {
	Bell() {
		envelope.attackTime = 0.01;
		envelope.decayTime = 1.0;
		envelope.sustainAmplitude = 0.0;
		envelope.releaseTime = 1.0;
	}

	double GetSound(double runTime, double hz) {
		return envelope.GetAmplitude(runTime) * (
			Oscillator(runTime, frequencyOutput * 2.0, Sine, 5.0, 0.001)
			+ 0.5 * Oscillator(runTime, frequencyOutput * 3.0, Sine)
			+ 0.25 * Oscillator(runTime, frequencyOutput * 4.0, Sine)
		);
	}
};

struct Harmonica : public Instrument {
	Harmonica() {
		envelope.attackTime = 0.10;
		envelope.decayTime = 0.1;
		envelope.sustainAmplitude = 0.8;
		envelope.releaseTime = 0.20;
	}

	double GetSound(double runTime, double hz) {
		return envelope.GetAmplitude(runTime) * (
			Oscillator(runTime, frequencyOutput, Square, 5.0, 0.001)
			+ 0.5 * Oscillator(runTime, frequencyOutput * 1.5, Square)
			+ 0.25 * Oscillator(runTime, frequencyOutput * 2.0, Square)
			+ 0.05 * Oscillator(runTime, 0, Noise)
			);
	}
};

Instrument *voice{ nullptr };

double HzToW(double hz) {
	return 2.0 * PI * hz;
}

double Oscillator(double runTime, double hz, waveTypes waveType, double lowFreqHz, double lowFreqAmp) {
	double modFreq{ HzToW(hz) * runTime + hz * lowFreqAmp * sin(HzToW(lowFreqHz) * runTime) };

	switch (waveType) {
	case Sine:
		return sin(modFreq);

	case Square:
		// return fmod(runTime, 1.0 / hz) < 1.0 / (2.0 * hz) ? 1.0 : -1.0;
		return sin(modFreq) > 0.0 ? 1.0 : -1.0;

	case AnalogSquare: {
		double output{ 0.0 };

		for (double i = 1.0; i < 100.0; i++)
			output += sin(modFreq * (2.0 * i - 1.0)) / (2.0 * i - 1.0);

		return (4.0 / PI) * output;
	}
			
	case Triangle:
		return asin(sin(modFreq)) * 2.0 / PI;
		
	case AnalogSawtooth: {
		double output{ 0.0 };

		for (double n = 1.0; n < 100.0; n++)
			output += sin(n * modFreq) / n;

		return -2.0 / PI * output;
	}

	case OptimalSawtooth:
		return 2.0 * hz * fmod(runTime, 1.0 / hz) - 1;

	default:
		return 2.0 * static_cast<double>(rand()) / static_cast<double>(RAND_MAX) - 1.0;
	}
}

double MakeNoise(double runTime) {
	return masterVolume * voice->GetSound(runTime, frequencyOutput);
}

int main() {
	vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

	for (auto device : devices)
		wcout << "Found output device: " << device << endl;

	voice = new Bell();

	olcNoiseMaker<short> sound(devices.at(0), 44100, 1, 8, 512);
	sound.SetUserFunction(MakeNoise);

	double octaveBaseFrequency{ 220.0 };
	double d12thRootOf2{ pow(2.0, 1.0 / 12.0) };

	int currentKey{ -1 };
	bool isKeyPressed { false };
	while (true) {
		isKeyPressed = false;

		for (int i = 0; i < 15; i++) {
			if (!(GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe"[i])) & 0x8000))
				continue;

			isKeyPressed = true;
			
			if (currentKey == i)
				continue;

			frequencyOutput = octaveBaseFrequency * pow(d12thRootOf2, i);
			voice->envelope.NotePressed(sound.GetTime());
			currentKey = i;
		}

		if (isKeyPressed || currentKey == -1)
			continue;

		voice->envelope.NoteReleased(sound.GetTime());
		currentKey = -1;
	}

	return 0;
}