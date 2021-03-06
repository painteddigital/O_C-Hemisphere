////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Base Class
////////////////////////////////////////////////////////////////////////////////

const int LEFT_HEMISPHERE = 0;
const int RIGHT_HEMISPHERE = 1;
const int HEMISPHERE_MAX_CV = 7800;
const int HEMISPHERE_CLOCK_TICKS = 100; // 6ms
const int HEMISPHERE_CURSOR_TICKS = 12000;

// Codes for help system sections
const int HEMISPHERE_HELP_DIGITALS = 0;
const int HEMISPHERE_HELP_CVS = 1;
const int HEMISPHERE_HELP_OUTS = 2;
const int HEMISPHERE_HELP_ENCODER = 3;

// Simulated fixed floats by multiplying and dividing by powers of 2
#define int2simfloat(x) (x << 14)
#define simfloat2int(x) (x >> 14)
typedef int32_t simfloat;

#define BottomAlign(h) (62 - h)

class HemisphereApplet {
public:

    virtual const char* applet_name(); // Maximum of 10 characters
    virtual void View();

    void IO(bool forwarding) {
        forwarding_on = (forwarding && hemisphere == RIGHT_HEMISPHERE);
        int fwd = forwarding_on ? io_offset : 0;
        for (int ch = 0; ch < 2; ch++)
        {
            // Set or forward CV inputs
            ADC_CHANNEL channel = (ADC_CHANNEL)(ch + io_offset - fwd);
            inputs[ch] = OC::ADC::raw_pitch_value(channel);

            // Handle clock timing
            if (clock_countdown[ch] > 0) {
                if (--clock_countdown[ch] == 0) Out(ch, 0);
            }
        }

        // Cursor countdown. See CursorBlink(), ResetCursor(), and gfxCursor()
        if (--cursor_countdown < -HEMISPHERE_CURSOR_TICKS) cursor_countdown = HEMISPHERE_CURSOR_TICKS;
    }

    /* Assign the child class instance to a left or right hemisphere */
    void SetHemisphere(int h) {
        hemisphere = h;
        gfx_offset = h * 65;
        io_offset = h * 2;
        forwarding_on = false;
        clock_countdown[0] = 0;
        clock_countdown[1] = 0;
        help_active = 0;
    }

    /* Help Screen Toggle */
    void HelpScreen() {
        help_active = 1 - help_active;
    }

    bool CursorBlink() {
        return cursor_countdown > 0;
    }

    void ResetCursor() {
        cursor_countdown = HEMISPHERE_CURSOR_TICKS;
    }

    void BaseView() {
        if (help_active) {
            // If help is active, draw the help screen instead of the application screen
            DrawHelpScreen();
        } else {
            View();
            DrawNotifications();
        }
    }

    //////////////// Notifications from the base class regarding manager state(s)
    ////////////////////////////////////////////////////////////////////////////////
    void DrawNotifications() {
        // CV Forwarding Icon
        if (forwarding_on) {
            graphics.setPrintPos(61, 2);
            graphics.print(">");
            graphics.setPrintPos(59, 2);
            graphics.print(">");
        }
    }

    void DrawHelpScreen() {
        gfxHeader(applet_name());
        SetHelp();
        for (int section = 0; section < 4; section++)
        {
            int y = section * 12 + 16;
            graphics.setPrintPos(0, y);
            if (section == HEMISPHERE_HELP_DIGITALS) graphics.print("Dig");
            if (section == HEMISPHERE_HELP_CVS) graphics.print("CV");
            if (section == HEMISPHERE_HELP_OUTS) graphics.print("Out");
            if (section == HEMISPHERE_HELP_ENCODER) graphics.print("Enc");
            graphics.invertRect(0, y - 1, 19, 9);

            graphics.setPrintPos(20, y);
            graphics.print(help[section]);
        }
    }

    //////////////// Offset graphics methods
    ////////////////////////////////////////////////////////////////////////////////
    void gfxCursor(int x, int y, int w) {
        if (CursorBlink()) gfxLine(x, y, x + w - 1, y);
    }

    void gfxPrint(int x, int y, const char *str) {
        graphics.setPrintPos(x + gfx_offset, y);
        graphics.print(str);
    }

    void gfxPrint(int x, int y, int num) {
        graphics.setPrintPos(x + gfx_offset, y);
        graphics.print(num);
    }

    void gfxPrint(const char *str) {
        graphics.print(str);
    }

    void gfxPrint(int num) {
        graphics.print(num);
    }

    void gfxPixel(int x, int y) {
        graphics.setPixel(x + gfx_offset, y);
    }

    void gfxFrame(int x, int y, int w, int h) {
        graphics.drawFrame(x + gfx_offset, y, w, h);
    }

    void gfxRect(int x, int y, int w, int h) {
        graphics.drawRect(x + gfx_offset, y, w, h);
    }

    void gfxInvert(int x, int y, int w, int h) {
        graphics.invertRect(x + gfx_offset, y, w, h);
    }

    void gfxLine(int x, int y, int x2, int y2) {
        graphics.drawLine(x + gfx_offset, y, x2 + gfx_offset, y2);
    }

    void gfxCircle(int x, int y, int r) {
        graphics.drawCircle(x + gfx_offset, y, r);
    }

    //////////////// Hemisphere-specific graphics methods
    ////////////////////////////////////////////////////////////////////////////////
    void gfxOutputBar(int ch, int y, bool screensaver) {
        int width = ProportionCV(outputs[ch], 60);
        if (width < 0) {width = 0;}
        int height = screensaver ? 2 : 12;
        int x = (hemisphere == 0) ? 64 - width : 0;
        gfxRect(x, y, width, height);
    }

    void gfxInputBar(int ch, int y, bool screensaver) {
        int width = ProportionCV(inputs[ch], 63);
        if (width < 0) {width = 0;}
        int height = screensaver ? 1 : 6;
        int x = (hemisphere == 0) ? 63 - width : 0;
        gfxFrame(x, y, width, height);
    }

    /* Original butterfly: functional grouping (ins with outs) */
    void gfxButterfly(bool screensaver) {
        for (int ch = 0; ch < 2; ch++)
        {
            gfxInputBar(ch, 15 + (ch * 10), screensaver);
            gfxOutputBar(ch, 35 + (ch * 15), screensaver);
        }
    }

    /* Alternate butterfly: Channel grouping (Ch1 with Ch2) */
    void gfxButterfly_Channel(bool screensaver) {
        for (int ch = 0; ch < 2; ch++)
        {
            gfxInputBar(ch, 15 + (ch * 25), screensaver);
            gfxOutputBar(ch, 25 + (ch * 25), screensaver);
        }
    }

    void gfxHeader(const char *str) {
        gfxPrint(1, 2, str);
        gfxLine(0, 10, 62, 10);
        gfxLine(0, 12, 62, 12);
    }

    //////////////// Offset I/O methods
    ////////////////////////////////////////////////////////////////////////////////
    int In(int ch) {
        return inputs[ch];
    }

    void Out(int ch, int value, int octave) {
        DAC_CHANNEL channel = (DAC_CHANNEL)(ch + io_offset);
        OC::DAC::set_pitch(channel, value, octave);
        outputs[ch] = value;
    }

    void Out(int ch, int value) {
        Out(ch, value, 0);
    }

    bool Clock(int ch) {
        bool clocked = 0;
        if (hemisphere == 0) {
            if (ch == 0) clocked = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>();
            if (ch == 1) clocked = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_2>();
        }
        if (hemisphere == 1) {
            if (ch == 0) clocked = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_3>();
            if (ch == 1) clocked = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_4>();
        }
        return clocked;
    }

    int Gate(int ch) {
        bool gated = 0;
        if (hemisphere == 0) {
            if (ch == 0) gated = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_1>();
            if (ch == 1) gated = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_2>();
        }
        if (hemisphere == 1) {
            if (ch == 0) gated = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_3>();
            if (ch == 1) gated = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_4>();
        }
        return gated;
    }

    void ClockOut(int ch, int ticks) {
        clock_countdown[ch] = ticks;
        Out(ch, 0, 5);
    }

    void ClockOut(int ch) {
        ClockOut(ch, HEMISPHERE_CLOCK_TICKS);
    }

protected:
    const char* help[4];
    virtual void SetHelp();

    //////////////// Calculation methods
    ////////////////////////////////////////////////////////////////////////////////

    /* Proportion method using simfloat, useful for calculating scaled values given
     * a fractional value.
     *
     * Solves this:  numerator        ???
     *              ----------- = -----------
     *              denominator       max
     *
     * For example, to convert a parameter with a range of 1 to 100 into value scaled
     * to HEMISPHERE_MAX_CV, to be sent to the DAC:
     *
     * Out(ch, Proportion(value, 100, HEMISPHERE_MAX_CV));
     *
     */
    int Proportion(int numerator, int denominator, int max_value) {
        simfloat proportion = int2simfloat((int32_t)numerator) / (int32_t)denominator;
        int scaled = simfloat2int(proportion * max_value);
        return scaled;
    }

    /* Proportion CV values into pixels for display purposes.
     *
     * Solves this:     cv_value           ???
     *              ----------------- = ----------
     *              HEMISPHERE_MAX_CV   max_pixels
     */
    int ProportionCV(int cv_value, int max_pixels) {
        return Proportion(cv_value, HEMISPHERE_MAX_CV, max_pixels);
    }

private:
    int hemisphere; // Which hemisphere (0, 1) this applet uses
    int gfx_offset; // Graphics offset, based on the side
    int io_offset; // Input/Output offset, based on the side
    int inputs[2];
    int outputs[2];
    int clock_countdown[2];
    int cursor_countdown;
    bool forwarding_on; // Forwarding was on during the last ISR cycle

    int help_active;
};
