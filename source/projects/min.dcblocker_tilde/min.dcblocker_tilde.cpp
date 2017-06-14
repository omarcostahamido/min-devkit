/// @file	
///	@ingroup 	minexamples
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Timothy Place
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"

using namespace c74::min;

class dcblocker : public object<dcblocker>, public vector_operator {
public:

	MIN_DESCRIPTION {	"Filter out DC offset. "
						"The DC offset or <a href='https://en.wikipedia.org/wiki/DC_bias'>DC bias</a>, is removed by applying a first-order highpass filter to the input."
						"<br/><br/>"
						"This first-order highpass filter algorithm is used pretty much everywhere (STK, ChucK, RTCMix, SuperCollider, Max, Pd, etc),"
						"with the difference equation:"
						"<br/><br/>"
						"	y(n) = (1 * x(n))  +  (-1 * x(n-1))  -  (-0.9997 * y(n-1))  ,  n = 0, 1, 2, 3, ..."
						"<br/><br/>"
						"which can be simplified to:"
						"<br/><br/>"
						"	y(n) = x(n) - x(n-1) + (0.9997 * y(n-1))"
						"<br/><br/>"
						"and thus characterized by the Z-transform:"
						"<br/><br/>"
						"	Y(z) = X(z)  -  X(z) * z^(-1)  +  Y(z) * 0.9997 * z^(-1)"
						"<br/><br/>"
						"meaning the transfer function is:"
						"<br/><br/>"
						"	H(z) = [1  -  z^(-1)]  /  [1  +  0.9997 * z^(-1)]"
						"<br/><br/>"
						"and resulting in the frequency response:"
						"<br/><br/>"
						"	H( e^(i*omega*T) ) = [1  -  e^(-i * omega * T)]  /  [1  +  0.9997 * e^(-i * omega * T)]"
						"<br/><br/>"
						"where $i$ is the sqrt(-1), e is Euler's log base, T is the sampling interval, and omega is 2*pi*frequency."
						"<br/><br/>"
						"In Max, it usually shows up simply as [biquad~ 1.0 -1.0 0.0 -0.9997 0.0]."
						"In other places it usually shows up with the feedback coefficient set to -0.995"
						"(e.g in SuperCollider and in [JOS, 2007, pp 273])."
						"The higher coefficient is desirable so as to not attenuate lowish frequencies in the spectrum,"
						"but with the caveat that it also won't respond as quickly to varying amounts DC Offset."
						"<br/><br/>"
						"The power is attenuated by -3 dB at a normalized frequency of 0.1612 * pi @ 0.9997."
						"At fs=44100 this translates to cf = 22050 * 0.1612 = 3554.46 Hz."
						"<br/><br/>"
						"The power is attenuated by -3 dB at a normalized frequency of 0.1604 * pi @ 0.995."
						"At fs=44100 this translates to cf = 22050 * 0.1604 = 3536.82 Hz."
						"<br/><br/>"
						"For reference, in this last case, the power is attenuated by -6 db (magnitude attenuated by -12 dB) @ 0.0798 * pi,"
						"which at fs=44100 translates to 1759.59 Hz."
	};
	MIN_TAGS		{	"audio, filters"		};
	MIN_AUTHOR		{	"Cycling '74"			};
	MIN_RELATED		{	"biquad~, filterdesign"	};


	// Because our object defines a constructor (below) this argument definition is for
	// documentation purposes only.
	argument<int> channelcount_arg { this, "channel_count", "The number of channels to process." };


	dcblocker(const atoms& args = {}) {
		if (!args.empty())
			m_channelcount = args[0];

		for (auto i=0; i<m_channelcount; ++i) {
			m_inlets.push_back(	 std::make_unique<inlet<>>(this, "(signal) audio input") );
			m_outlets.push_back( std::make_unique<outlet<>>(this, "(signal) audio output", "signal") );
			m_filters.push_back( std::make_unique<lib::dcblocker>() );
		}
	}


	message<> clear { this, "clear",
		"Reset the DC-Blocking filter. Because this is an IIR filter it has the potential to blow-up, requiring a reset.",
		MIN_FUNCTION {
			for (auto& filter : m_filters)
				filter->clear();
			return {};
		}
	};


	attribute<bool>	bypass { this, "bypass" , false, description{"Pass the input straight-through."} };


	/// Process N channels of audio
	/// Max takes care of squashing denormal for us by setting the FTZ bit on the CPU.

	void operator()(audio_bundle input, audio_bundle output) {
		if (bypass)
			output = input;
		else {
			for (auto channel=0; channel<m_channelcount; ++channel) {
				auto	x = input.samples(channel);
				auto	y = output.samples(channel);
				auto&	f = *m_filters[channel];

				for (auto i=0; i<input.framecount(); ++i) {
					y[i] = f(x[i]);
				}
			}
		}
	}

private:
	int										m_channelcount = 1;		///< number of channels
	vector< unique_ptr<inlet<>> >			m_inlets;				///< this object's inlets
	vector< unique_ptr<outlet<>> >			m_outlets;				///< this object's outlets
	vector< unique_ptr<lib::dcblocker> >	m_filters;				///< dc-blocking filters for each channel
};

MIN_EXTERNAL(dcblocker);
