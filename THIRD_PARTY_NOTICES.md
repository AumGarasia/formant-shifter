# Third-party notices

| Component | Version | Licence | Used for |
|---|---|---|---|
| JUCE | 8.0.12 | See https://juce.com/legal (JUCE EULA or AGPLv3) | Plugin framework, UI |
| Signalsmith Stretch | 1.4.0 | MIT | Pitch and formant shifting engine |
| Signalsmith Linear | 0.6.4 | MIT | FFT and STFT, used by Signalsmith Stretch |
| Catch2 | see `external/Catch2` | Boost Software Licence | Tests only, not shipped |
| pluginval | external tool | see its repository | Validation only, not shipped |

When distributing a build, include the licence texts of everything that ships. For the MIT
components that means `external/signalsmith-stretch/LICENSE.txt` and the licence file in
`external/signalsmith-linear`. Re-check the VST3 SDK terms for the VST3 format.