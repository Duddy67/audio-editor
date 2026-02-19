#include "file_io.h"
#include "track.h"

/*
 * Loads a given audio file.
 */
void FileIO::load(const char *filename, Buffer& buffer, const Engine& engine)
{
    printf("Load audio file '%s'\n", filename); // Debog.
    // First ensure the file format is supported.
    std::string fileFormat = std::filesystem::path(filename).extension();
    std::vector<std::string> supportedFormats = engine.getSupportedFormats();
    unsigned int size = supportedFormats.size();
    bool supported = false;

    // Check wether the format of the given file is supported
    for (unsigned int i = 0; i < size; i++) {
        if (fileFormat.compare(supportedFormats[i]) == 0) {
            supported = true;
            break;
        }
    }

    if (!supported) {
        throw std::runtime_error("Format: " + fileFormat + " not supported.");
    }

    // First set the original data file format.
    if (!setFormat(filename, buffer.getFormat())) {
        throw std::runtime_error("Failed to initialized temporary decoder.");
    }

    // Then initialize decoder with format conversion (except for output channels).
    ma_decoder_config decoderConfig = ma_decoder_config_init(engine.getDefaultOutputFormat(), buffer.getFormat().outputChannels, engine.getDefaultOutputSampleRate());

    if (ma_decoder_init_file(filename, &decoderConfig, &decoder) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize decoder with conversion.");
    }

    if (!decode(buffer)) {
        throw std::runtime_error("Failed to decode file.");
    }

    ma_decoder_uninit(&decoder);
}

/*
 * Decode the entire file manually to playback straight from memory (ie: no streaming).
 */
bool FileIO::decode(Buffer& buffer)
{
    ma_uint64 frameCount = 0;

    if (ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount) != MA_SUCCESS) {
        std::cerr << "Failed to get length" << std::endl;
        ma_decoder_uninit(&decoder);
        return false;
    }

    // Create a array/buffer to hold the total number of samples (not frames!):
    // nb frames * nb channels = total nb samples
    std::vector<float> tempData(static_cast<size_t>(frameCount * decoder.outputChannels));

    ma_uint64 framesRead = 0;
    if (ma_decoder_read_pcm_frames(&decoder, tempData.data(), frameCount, &framesRead) != MA_SUCCESS) {
        std::cerr << "Failed to read PCM frames" << std::endl;
        ma_decoder_uninit(&decoder);
        return false;
    }

    // Check whether the file is stereo.
    ma_uint64 totalFrames = static_cast<size_t>(framesRead);

    // Write audio data into buffer.
    buffer.clear();
    buffer.reserve(totalFrames);
    buffer.fillFromInterleaved(tempData, totalFrames);

    return true;
}

void FileIO::save(const char* filename, Buffer& buffer)
{
    ma_encoder_config config = ma_encoder_config_init(
        ma_encoding_format_wav,
        ma_format_f32,      // 32-bit float samples
        2,                  // stereo
        44100               // sample rate (adjust to your app)
    );

    ma_encoder encoder;
    if (ma_encoder_init_file(filename, &config, &encoder) != MA_SUCCESS) {
        printf("Failed to initialize encoder.\n");
        return;
    }

    std::vector<float> interleaved;
    buffer.interleaveSamples(interleaved);
    size_t frameCount = buffer.getLeftSamples().size();

    // Write audio data
    ma_uint64 framesWritten = 0;
    ma_encoder_write_pcm_frames(&encoder, interleaved.data(), frameCount, &framesWritten);

    // Clean up
    ma_encoder_uninit(&encoder);

    printf("Wrote %llu frames to %s\n", framesWritten, filename);
}

/*
 * Probes the original file format and store its data.
 */
bool FileIO::setFormat(const char* filename, Format& format)
{
    // Initialize a temporary decoder without any config data (ie: NULL).
    ma_decoder decoderProbe;

    if (ma_decoder_init_file(filename, NULL, &decoderProbe) != MA_SUCCESS) {
        ma_decoder_uninit(&decoderProbe);
        return false;
    }

    // Retrieve data about the original file format.
    format.fileName = filename;
    format.outputChannels = decoderProbe.outputChannels;
    format.outputSampleRate = decoderProbe.outputSampleRate;
    format.outputFormat = decoderProbe.outputFormat;

    // Done probing
    ma_decoder_uninit(&decoderProbe);

    return true;
}

void FileIO::setNewFileFormat(Format& format, bool stereo, const Engine& engine)
{
    format.outputChannels = stereo ? 2 : 1;
    format.outputSampleRate = engine.getDefaultOutputSampleRate();
    format.outputFormat = engine.getDefaultOutputFormat();
}

