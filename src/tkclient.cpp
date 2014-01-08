#include <tcl.h>
#include <string.h>
#include <sstream>
#include "queue.h"
#include "trackdef.h"
#include "fantomdef.h"
#include "xml.h"

class EvalException
{
public:
    int m_rv;
    Tcl_Interp *m_interp;
    EvalException(int rv, Tcl_Interp *interp): m_rv(rv), m_interp(interp) { }
};

class TclEval
{
    std::stringstream m_sstream;
public:
    std::stringstream &stream() { return m_sstream; }
    void flush(Tcl_Interp *interp)
    {
        std::string s(m_sstream.str());
        Tcl_Obj *obj = Tcl_NewStringObj(s.c_str(), -1);
        m_sstream.str(std::string());
        int rv = Tcl_EvalObjEx(interp, obj, TCL_EVAL_DIRECT|TCL_EVAL_GLOBAL);
        if (rv != TCL_OK)
        {
            printf("command is \"%s\"\n", s.c_str());
            throw EvalException(rv, interp);
        }
    }
    void flush(Tcl_Interp *interp, int &i)
    {
        flush(interp);
        Tcl_Obj *id = Tcl_GetObjResult(interp);
        int rv = Tcl_GetIntFromObj(interp, id, &i);
        if (rv != TCL_OK)
        {
            throw EvalException(rv, interp);
        }
    }
};

class Banner
{
public:
    const char *m_canvas;
    int m_row;
    int m_col;
    struct {
        int m_value;
        int m_fraction;
    } m_id;
    Banner(Tcl_Interp *interp, const char *canvas, const char *field, int row, int col): 
        m_canvas(canvas), m_row(row), m_col(col)
    {
        TclEval eval;
        eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text {" 
            << field << "} -anchor w";
        eval.flush(interp);
        update(interp, "banner", 0, 1);
    }
    void clear(Tcl_Interp *interp)
    {
        TclEval eval;
        eval.stream() << m_canvas << " delete " << m_id.m_value;
        eval.flush(interp);
        eval.stream() << m_canvas << " delete " << m_id.m_fraction;
        eval.flush(interp);
    }
    void update(Tcl_Interp *interp, const char *value, int numerator, int denominator)
    {
        int row = m_row;
        int col = m_col;
        clear(interp);
        TclEval eval;
        col += 60;
        eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text {" 
            << numerator << "/" << denominator << "}";
        eval.flush(interp, m_id.m_fraction);
        col += 80;
        eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text {" 
            << value << "} -anchor w -font TkTextFont";
        eval.flush(interp, m_id.m_value);
    }
};

class Range
{
public:
    const char *m_canvas;
    int m_x1;
    int m_y1;
    int m_x2;
    int m_y2;
    int m_value1;
    int m_value2;
    static const int m_min = Midi::Note::E2;
    static const int m_max = Midi::Note::G8;
    struct {
        int m_bg;
        int m_fg;
        int m_text1;
        int m_text2;
    } m_id;
    static int clamp(int x, int xa, int xb)
    {
        return std::max(xa, std::min(x, xb));
    }
    Range(Tcl_Interp *interp, const char *canvas, int x1, int y1, int x2, int y2,
            int value1, int value2):
        m_canvas(canvas), m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2),
        m_value1(value1), m_value2(value2)
    {
        m_value1 = clamp(m_value1, m_min, m_max);
        m_value2 = clamp(m_value2, m_min, m_max);
        m_value1 -= m_min;
        m_value2 -= m_min;
        double range = m_max - m_min;
        TclEval eval;
        eval.stream() << m_canvas << " create rectangle " << m_x1 << "p " << m_y1 <<
                "p " << m_x2 << "p " << m_y2 << "p -fill black";
        eval.flush(interp, m_id.m_bg);
        double f1 = (double)m_value1/range;
        double f2 = (double)m_value2/range;
        int x3 = m_x1 + f1 * (m_x2-m_x1);
        int x4 = m_x1 + f2 * (m_x2-m_x1);
        if (std::abs(x3-x4) < 2)
        {
            if (x3 > m_min)
                x3 -= 2;
            else
                x4 += 2;
        }
        eval.stream() << m_canvas << " create rectangle " << x3 << "p " << m_y1 <<
                "p " << x4 << "p " << m_y2 << "p -fill blue";
        eval.flush(interp, m_id.m_fg);
        int y3 = (m_y1 + m_y2)/2;
        eval.stream() << m_canvas << " create text " << m_x1 << "p " << y3 << "p -text {" << Midi::noteName(m_value1+m_min) << "} -fill white -anchor w";
        eval.flush(interp, m_id.m_text1);
        eval.stream() << m_canvas << " create text " << m_x2 << "p " << y3 << "p -text {" << Midi::noteName(m_value2+m_min) << "} -fill white -anchor e";
        eval.flush(interp, m_id.m_text2);
    }
    void clear(Tcl_Interp *interp)
    {
        TclEval eval;
        eval.stream() << m_canvas << " delete " << m_id.m_fg;
        eval.flush(interp);
        eval.stream() << m_canvas << " delete " << m_id.m_bg;
        eval.flush(interp);
        eval.stream() << m_canvas << " delete " << m_id.m_text1;
        eval.flush(interp);
        eval.stream() << m_canvas << " delete " << m_id.m_text2;
        eval.flush(interp);
    }
};

class Bar
{
public:
    const char *m_canvas;
    int m_x1;
    int m_y1;
    int m_x2;
    int m_y2;
    int m_value;
    int m_max;
    struct {
        int m_bg;
        int m_fg;
        int m_text;
    } m_id;
    Bar(Tcl_Interp *interp, const char *canvas, int x1, int y1, int x2, int y2, int value, int max):
        m_canvas(canvas), m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2), m_value(value), m_max(max)
    {
        TclEval eval;
        eval.stream() << m_canvas << " create rectangle " << m_x1 << "p " << m_y1 <<
                "p " << m_x2 << "p " << m_y2 << "p -fill black";
        eval.flush(interp, m_id.m_bg);
        double f = (double)m_value/(double)m_max;
        int x3 = m_x1 + f * (m_x2-m_x1);
        eval.stream() << m_canvas << " create rectangle " << m_x1 << "p " << m_y1 <<
                "p " << x3 << "p " << m_y2 << "p -fill green";
        eval.flush(interp, m_id.m_fg);
        x3 = (m_x1 + m_x2)/2;
        int y3 = (m_y1 + m_y2)/2;
        eval.stream() << m_canvas << " create text " << x3 << "p " << y3 << "p -text " << m_value << " -fill white";
        eval.flush(interp, m_id.m_text);
    }
    void update(Tcl_Interp *interp, int value)
    {
        m_value = value;
        TclEval eval;
        eval.stream() << m_canvas << " delete " << m_id.m_fg;
        eval.flush(interp);
        double f = (double)m_value/(double)m_max;
        int x3 = m_x1 + f * (m_x2-m_x1);
        eval.stream() << m_canvas << " create rectangle " << m_x1 << "p " << m_y1 <<
                "p " << x3 << "p " << m_y2 << "p -fill green";
        eval.flush(interp, m_id.m_fg);
        eval.stream() << m_canvas << " dchars " << m_id.m_text << " 0 end";
        eval.flush(interp);
        x3 = (m_x1 + m_x2)/2;
        int y3 = (m_y1 + m_y2)/2;
        eval.stream() << m_canvas << " create text " << x3 << "p " << y3 << "p -text " << m_value << " -fill white";
        eval.flush(interp, m_id.m_text);
    }
};

class SwPartState
{
    const char *m_canvas;
    Range *m_range;
    int m_num;
    int m_num1;
    int m_num2;
    struct {
        int m_num;
        int m_channel;
        int m_preset;
        int m_name;
    } m_id;
    SwPart *swPart() const;
    Fantom::Part *hwPart() const;
public:
    SwPartState(Tcl_Interp *interp, const char *canvas, int num, int num1, int num2);
    void update(Tcl_Interp *interp);
    void clear(Tcl_Interp *interp);
};

class HwPartState
{
    const char *m_canvas;
    Bar *m_volumeBar;
    int m_num;
    struct {
        int m_num;
        int m_channel;
        int m_preset;
        int m_name;
    } m_id;
    Fantom::Part *hwPart() const;
public:
    HwPartState(Tcl_Interp *interp, const char *canvas, int num);
    void update(Tcl_Interp *interp);
};

class BannerState
{
    Banner *m_track;
    Banner *m_section;
public:
    BannerState(Tcl_Interp *interp, const char *canvas)
    {
        m_track = new Banner(interp, canvas, "track", 15, 50);
        m_section = new Banner(interp, canvas, "section", 60, 50);
    }
    void update(Tcl_Interp *interp);
};

class TkClientState
{
public:
    TrackList m_trackList;
    SetList m_setList;
    Event m_event;
    int m_currentTrack;
    int m_currentSection;
    HwPartState *m_hwPartState[16];
    SwPartState *m_swPartState[64];
    BannerState *m_bannerState;
    Track *currentTrack() const {
        return m_trackList[m_currentTrack];
    }
    Section *currentSection() const {
        return currentTrack()->m_sectionList[m_currentSection];
    }
    void init()
    {
        m_currentTrack = -1;
        m_currentSection = -1;
        m_bannerState = 0;
        for (int i=0; i<16; i++)
            m_hwPartState[i] = 0;
        for (int i=0; i<64; i++)
            m_swPartState[i] = 0;
    }
};

TkClientState tkClientState;

void BannerState::update(Tcl_Interp *interp)
{
    m_track->update(interp, tkClientState.currentTrack()->m_name, 
        (tkClientState.m_currentTrack+1), tkClientState.m_trackList.size());
    m_section->update(interp, tkClientState.currentSection()->m_name, 
        (tkClientState.m_currentSection+1), tkClientState.currentTrack()->m_sectionList.size());
}

Fantom::Part *HwPartState::hwPart() const
{
    return &tkClientState.currentTrack()->m_performance->m_partList[m_num];
}

SwPart *SwPartState::swPart() const
{
    return tkClientState.currentSection()->m_partList[m_num1];
}

Fantom::Part *SwPartState::hwPart() const
{
    return swPart()->m_hwPartList[m_num2];
}

SwPartState::SwPartState(Tcl_Interp *interp, const char *canvas, int num, int num1, int num2):
            m_canvas(canvas), m_range(0), m_num(num), m_num1(num1), m_num2(num2)
{
    int row = (m_num % 4) * 30 + 30;
    int col = (m_num / 4) * 380 + 30;
    TclEval eval;
    eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text " << (1+m_num);
    eval.flush(interp, m_id.m_num);
    col += 30;
    eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text " << (1+swPart()->m_channel);
    eval.flush(interp, m_id.m_channel);
    col += 40;
    eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text {" << hwPart()->m_preset << "}";
    eval.flush(interp, m_id.m_preset);
    col += 60;
    eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text {" << hwPart()->m_patch.m_name << "} -anchor w";
    eval.flush(interp, m_id.m_name);
    col += 90;
    m_range = new Range(interp, canvas, col, row-5, col+100, row+5, swPart()->m_rangeLower, swPart()->m_rangeUpper);
}

HwPartState::HwPartState(Tcl_Interp *interp, const char *canvas, int num): m_canvas(canvas), m_volumeBar(0),
    m_num(num)
{
    int row = (m_num % 8) * 30 + 15;
    int col = (m_num / 8) * 380 + 30;
    TclEval eval;
    eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text " << (1+m_num);
    eval.flush(interp, m_id.m_num);
    col += 30;
    eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text " << (1+hwPart()->m_channel);
    eval.flush(interp, m_id.m_channel);
    col += 40;
    eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text {" << hwPart()->m_preset << "}";
    eval.flush(interp, m_id.m_preset);
    col += 60;
    eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text {" << hwPart()->m_patch.m_name << "} -anchor w";
    eval.flush(interp, m_id.m_name);
    col += 90;
    m_volumeBar = new Bar(interp,  canvas, col, row-5, col+100, row+5, hwPart()->m_volume, 127);
}

void SwPartState::update(Tcl_Interp *interp)
{
    TclEval eval;
    eval.stream() << m_canvas << " insert " << m_id.m_num << " end " << (1+m_num);
    eval.flush(interp);
    eval.stream() << m_canvas << " insert " << m_id.m_channel << " end {" << (int)(1+hwPart()->m_channel) << "}";
    eval.flush(interp);
    eval.stream() << m_canvas << " insert " << m_id.m_preset << " end {" << hwPart()->m_preset << "}";
    eval.flush(interp);
    eval.stream() << m_canvas << " insert " << m_id.m_name << " end {" << hwPart()->m_patch.m_name << "}";
    eval.flush(interp);
    int row = (m_num % 8) * 30 + 30;
    int col = (m_num / 8) * 380 + 30 + 30 +40 +60 +90;
    m_range = new Range(interp, m_canvas, col, row-5, col+100, row+5, swPart()->m_rangeLower, swPart()->m_rangeUpper);
}

void SwPartState::clear(Tcl_Interp *interp)
{
    TclEval eval;
    eval.stream() << m_canvas << " dchars " << m_id.m_channel << " 0 end";
    eval.flush(interp);
    eval.stream() << m_canvas << " dchars " << m_id.m_num << " 0 end";
    eval.flush(interp);
    eval.stream() << m_canvas << " dchars " << m_id.m_channel << " 0 end";
    eval.flush(interp);
    eval.stream() << m_canvas << " dchars " << m_id.m_preset << " 0 end";
    eval.flush(interp);
    eval.stream() << m_canvas << " dchars " << m_id.m_name << " 0 end";
    eval.flush(interp);
    if (m_range)
    {
        m_range->clear(interp);
        delete m_range;
    }
    m_range = 0;
}

void HwPartState::update(Tcl_Interp *interp)
{
    TclEval eval;
    eval.stream() << m_canvas << " dchars " << m_id.m_channel << " 0 end";
    eval.flush(interp);
    eval.stream() << m_canvas << " insert " << m_id.m_channel << " end " << (int)(1+hwPart()->m_channel) << "";
    eval.flush(interp);
    eval.stream() << m_canvas << " dchars " << m_id.m_preset << " 0 end";
    eval.flush(interp);
    eval.stream() << m_canvas << " insert " << m_id.m_preset << " end {" << hwPart()->m_preset << "}";
    eval.flush(interp);
    eval.stream() << m_canvas << " dchars " << m_id.m_name << " 0 end";
    eval.flush(interp);
    eval.stream() << m_canvas << " insert " << m_id.m_name << " end {" << hwPart()->m_patch.m_name << "}";
    eval.flush(interp);
    m_volumeBar->update(interp, hwPart()->m_volume);
}

int getSet(Tcl_Interp *interp)
{
    Tcl_Obj *setList = Tcl_NewObj();
    Tcl_SetObjResult(interp, setList);
    for (int i=0; i<tkClientState.m_setList.size(); i++)
    {
        Tcl_AppendPrintfToObj(setList, "%d ", tkClientState.m_setList[i]);
    }
    return TCL_OK;
}

int processSectionChange(Tcl_Interp *interp, uint8_t newSection)
{
    tkClientState.m_currentSection = newSection;
    int num = 0;
    for (int i=0; i<12 && tkClientState.m_swPartState[i]; i++)
    {
        tkClientState.m_swPartState[i]->clear(interp);
    }
    if (tkClientState.m_bannerState == 0)
        tkClientState.m_bannerState = new BannerState(interp, ".bannerCanvas");
    tkClientState.m_bannerState->update(interp);
    for (int num1 = 0; num < 12 &&
            num1 < (int)tkClientState.currentSection()->m_partList.size(); num1++)
    {
        for (int num2 = 0; num < 12 &&
                num2 < (int)tkClientState.currentSection()->m_partList[num1]->m_hwPartList.size();
                num2++, num++)
        {
            if (tkClientState.m_swPartState[num] == 0)
                tkClientState.m_swPartState[num] = new SwPartState(interp, ".c", num, num1, num2);
            else
            {
                tkClientState.m_swPartState[num]->update(interp);
            }
        }
    }
    return TCL_OK;
}

int processTrackChange(Tcl_Interp *interp, uint8_t newTrack)
{
    tkClientState.m_currentTrack = newTrack;
    for (int i=0; i<16; i++)
    {
        if (tkClientState.m_hwPartState[i] == 0)
            tkClientState.m_hwPartState[i] = new HwPartState(interp, ".fantomCanvas", i);
        else
        {
            tkClientState.m_hwPartState[i]->update(interp);
        }
    }
    return TCL_OK;
}

int processEvent(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    (void)cdata;
    (void)objc;
    unsigned int scans[11];
    int rv = sscanf(Tcl_GetStringFromObj(objv[1], 0),
                    "%x %x %x %x %x %x %x %x %x %x %x",
                    scans+0, scans+1, scans+2, scans+3,
                    scans+4, scans+5, scans+6, scans+7,
                    scans+8, scans+9, scans+10);
    if (rv != 11)
    {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("processEvent: cannot parse event", -1));
        return TCL_ERROR;
    }
    tkClientState.m_event.m_packetCounter = scans[0];
    tkClientState.m_event.m_metaMode = scans[1];
    tkClientState.m_event.m_currentTrack = scans[2];
    tkClientState.m_event.m_currentSection = scans[3];
    tkClientState.m_event.m_trackIdxWithinSet = scans[4];
    tkClientState.m_event.m_type = scans[5];
    tkClientState.m_event.m_deviceId = scans[6];
    tkClientState.m_event.m_part = scans[7];
    tkClientState.m_event.m_midi[0] = scans[8];
    tkClientState.m_event.m_midi[1] = scans[9];
    tkClientState.m_event.m_midi[2] = scans[10];
    try
    {
        bool forceSectionChange = false;
        if (tkClientState.m_event.m_currentTrack != tkClientState.m_currentTrack)
        {
            rv = processTrackChange(interp, tkClientState.m_event.m_currentTrack);
            if (rv != TCL_OK)
                return rv;
            forceSectionChange = true;
        }
        if (forceSectionChange || 
            tkClientState.m_event.m_currentSection != tkClientState.m_currentSection)
        {
            rv = processSectionChange(interp, tkClientState.m_event.m_currentSection);
            if (rv != TCL_OK)
                return rv;
        }
    }
    catch (EvalException &e)
    {
        return e.m_rv;
    }
    return TCL_OK;
}

int loadXml(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    (void)cdata;
    (void)interp;
    (void)objc;
    (void)objv;
    XML xml;
    xml.importTracks(TRACK_DEF, tkClientState.m_trackList,
                        tkClientState.m_setList);
    Fantom::PerformanceList performanceList;
    xml.importPerformances("fantom_cache.xml", performanceList);
    TrackList::iterator track = tkClientState.m_trackList.begin();
    Fantom::PerformanceList::iterator performance =
                performanceList.begin();
    for (; performance != performanceList.end();
            ++performance, ++track)
    {
        (*track)->merge(*performance);
    }
    return TCL_OK;
}

extern "C" {
int Patcher_Init(Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL)
    {
        return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, "Patcher", "1.0") == TCL_ERROR)
    {
        return TCL_ERROR;
    }
    tkClientState.init();
    Tcl_CreateObjCommand(interp, "loadxml", loadXml, NULL, NULL);
    Tcl_CreateObjCommand(interp, "processEvent", processEvent, NULL, NULL);
    return TCL_OK;
}
} // extern "C"
