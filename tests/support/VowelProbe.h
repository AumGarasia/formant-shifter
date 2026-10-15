#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// Synthetic-vowel measurement helpers, shared by the engine_probe tool and the
// formant tests. The spectral envelope is fixed and the harmonics sit on a chosen
// fundamental, so any change in the output's pitch or spectral shape is caused
// by the processing.
namespace vowelprobe
{

inline constexpr double kSampleRate = 44100.0;
inline constexpr double kPi = 3.14159265358979323846;
inline constexpr double kBaseF0 = 140.0;
inline constexpr std::size_t kLength = 3 * 44100;          // 3 seconds
inline constexpr std::size_t kMeasureStart = 2 * 44100;    // measure once the engine has settled
inline constexpr std::size_t kWindow = 8192;

// Fixed vowel-like spectral envelope: three resonances.
inline double envelope (double hz)
{
    struct Formant { double centre, bandwidth, gain; };
    constexpr Formant formants[] = { { 700.0, 120.0, 1.0 },
                                     { 1200.0, 150.0, 0.7 },
                                     { 2600.0, 200.0, 0.4 } };
    double sum = 0.0;

    for (const auto& f : formants)
    {
        const double x = (hz - f.centre) / f.bandwidth;
        sum += f.gain / (1.0 + x * x);
    }

    return sum;
}

// Additive synthesis: harmonics of f0 shaped by the envelope above.
inline std::vector<float> makeVowel (double f0 = kBaseF0)
{
    std::vector<float> out (kLength, 0.0f);

    for (int h = 1; h * f0 < 8000.0; ++h)
    {
        const double hz = h * f0;
        const double amp = 0.05 * envelope (hz);
        const double w = 2.0 * kPi * hz / kSampleRate;
        const double cw = std::cos (w), sw = std::sin (w);
        double c = 1.0, s = 0.0;                    // cos and sin of (w * n), by recurrence

        for (std::size_t n = 0; n < kLength; ++n)
        {
            out[n] += static_cast<float> (amp * s);
            const double nextC = c * cw - s * sw;
            s = c * sw + s * cw;
            c = nextC;
        }
    }

    return out;
}

// Fundamental frequency by autocorrelation (40 to 600 Hz search range), with
// parabolic interpolation of the peak.
inline double estimateF0 (const std::vector<float>& x)
{
    const auto minLag = static_cast<std::size_t> (kSampleRate / 600.0);
    const auto maxLag = static_cast<std::size_t> (kSampleRate / 40.0);

    std::vector<double> ac (maxLag + 2, 0.0);

    for (std::size_t lag = minLag; lag <= maxLag + 1; ++lag)
    {
        double sum = 0.0;

        for (std::size_t n = 0; n < kWindow; ++n)
            sum += static_cast<double> (x[kMeasureStart + n]) * static_cast<double> (x[kMeasureStart + n + lag]);

        ac[lag] = sum;
    }

    double best = 0.0;
    for (std::size_t lag = minLag; lag <= maxLag; ++lag)
        best = std::max (best, ac[lag]);

    // First local maximum that is close to the overall best: the fundamental period.
    for (std::size_t lag = minLag + 1; lag <= maxLag; ++lag)
    {
        if (ac[lag] >= 0.9 * best && ac[lag] >= ac[lag - 1] && ac[lag] >= ac[lag + 1])
        {
            const double a = ac[lag - 1], b = ac[lag], c = ac[lag + 1];
            const double denominator = a - 2.0 * b + c;
            const double shift = std::fabs (denominator) > 1e-12 ? 0.5 * (a - c) / denominator : 0.0;
            return kSampleRate / (static_cast<double> (lag) + shift);
        }
    }

    return 0.0;
}

// Magnitude spectrum below 5 kHz (DFT with a Hann window), indexed by bin number.
inline std::vector<double> magnitudeSpectrum (const std::vector<float>& x)
{
    const auto maxBin = static_cast<std::size_t> (5000.0 * static_cast<double> (kWindow) / kSampleRate);

    std::vector<double> windowed (kWindow);
    for (std::size_t n = 0; n < kWindow; ++n)
    {
        const double hann = 0.5 - 0.5 * std::cos (2.0 * kPi * static_cast<double> (n) / static_cast<double> (kWindow));
        windowed[n] = static_cast<double> (x[kMeasureStart + n]) * hann;
    }

    std::vector<double> magnitudes (maxBin + 1, 0.0);

    for (std::size_t k = 1; k <= maxBin; ++k)
    {
        const double w = 2.0 * kPi * static_cast<double> (k) / static_cast<double> (kWindow);
        const double cw = std::cos (w), sw = std::sin (w);
        double c = 1.0, s = 0.0;
        double re = 0.0, im = 0.0;

        for (std::size_t n = 0; n < kWindow; ++n)
        {
            re += windowed[n] * c;
            im -= windowed[n] * s;
            const double nextC = c * cw - s * sw;
            s = c * sw + s * cw;
            c = nextC;
        }

        magnitudes[k] = std::sqrt (re * re + im * im);
    }

    return magnitudes;
}

// Magnitude-weighted mean frequency: a rough proxy for where the formants sit.
inline double spectralCentroid (const std::vector<double>& magnitudes)
{
    double weighted = 0.0, total = 0.0;

    for (std::size_t k = 1; k < magnitudes.size(); ++k)
    {
        weighted += magnitudes[k] * static_cast<double> (k) * kSampleRate / static_cast<double> (kWindow);
        total += magnitudes[k];
    }

    return total > 0.0 ? weighted / total : 0.0;
}

inline double spectralCentroid (const std::vector<float>& x)
{
    return spectralCentroid (magnitudeSpectrum (x));
}

// Fraction of spectral power that lies close to a harmonic of f0 (0 to 1). A clean
// harmonic sound scores high; smearing and warble spread energy between the
// harmonics and pull it down. Only meaningful as a comparison against another
// measurement of the same kind of signal.
inline double harmonicPurity (const std::vector<double>& magnitudes, double f0)
{
    if (f0 <= 0.0)
        return 0.0;

    const double binHz = kSampleRate / static_cast<double> (kWindow);
    const double tolerance = std::max (12.0, 0.06 * f0);   // 12 Hz covers the Hann main lobe
    double harmonic = 0.0, total = 0.0;

    for (std::size_t k = 1; k < magnitudes.size(); ++k)
    {
        const double hz = static_cast<double> (k) * binHz;
        const double power = magnitudes[k] * magnitudes[k];
        const double nearest = std::round (hz / f0) * f0;

        total += power;

        if (nearest > 0.0 && std::fabs (hz - nearest) <= tolerance)
            harmonic += power;
    }

    return total > 0.0 ? harmonic / total : 0.0;
}

} // namespace vowelprobe