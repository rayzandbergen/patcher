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
        for (int i=0; i<16; i++)
            m_hwPartState[i] = 0;
        for (int i=0; i<64; i++)
            m_swPartState[i] = 0;
    }
};

TkClientState tkClientState;

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
            m_canvas(canvas), m_num(num), m_num1(num1), m_num2(num2)
{
    int row = (m_num % 4) * 30 + 30;
    int col = (m_num / 4) * 180 + 30;
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
    eval.stream() << m_canvas << " create text " << col << "p " << row << "p -text {" << hwPart()->m_patch.m_name << "}";
    eval.flush(interp, m_id.m_name);
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
    m_volumeBar = new Bar(interp,  ".fantomCanvas", col, row-5, col+100, row+5, hwPart()->m_volume, 127);
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

int parseGetPatch(Tcl_Interp *interp, int argc, int objc, Tcl_Obj *const objv[], const Fantom::Part *part)
{
    if (argc < objc)
    {
        const Fantom::Patch *patch = &part->m_patch;
        const char *member = Tcl_GetStringFromObj(objv[argc], 0);
        if (strcmp(member, "name") == 0)
            Tcl_SetObjResult(interp, Tcl_NewStringObj(patch->m_name, -1));
        else
        {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid patch field", -1));
            return TCL_ERROR;
        }
        return TCL_OK;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing patch field", -1));
    return TCL_ERROR;
}

int parseGetHwPart(Tcl_Interp *interp, int argc, int objc, Tcl_Obj *const objv[], const Fantom::Part *part)
{
    if (argc < objc)
    {
        const char *member = Tcl_GetStringFromObj(objv[argc], 0);
        if (strcmp(member, "channel") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_channel));
        else if (strcmp(member, "bankSelectMsb") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_bankSelectMsb));
        else if (strcmp(member, "bankSelectLsb") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_bankSelectLsb));
        else if (strcmp(member, "programChange") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_programChange));
        else if (strcmp(member, "preset") == 0)
            Tcl_SetObjResult(interp, Tcl_NewStringObj(part->m_preset, -1));
        else if (strcmp(member, "volume") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_volume));
        else if (strcmp(member, "transpose") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_transpose));
        else if (strcmp(member, "octave") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_octave));
        else if (strcmp(member, "keyRangeLower") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_keyRangeLower));
        else if (strcmp(member, "keyRangeUpper") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_keyRangeUpper));
        else if (strcmp(member, "fadeWidthLower") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_fadeWidthLower));
        else if (strcmp(member, "fadeWidthUpper") == 0)
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_fadeWidthUpper));
        // \todo complete member list
        else if (strcmp(member, "patch") == 0)
            return parseGetPatch(interp, ++argc, objc, objv, part);
        else
        {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid part field", -1));
            return TCL_ERROR;
        }
        return TCL_OK;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing part field", -1));
    return TCL_ERROR;
}

int parseGetHwPartList(Tcl_Interp *interp, int argc, int objc, Tcl_Obj *const objv[], const SwPart *swPart)
{
    if (argc < objc)
    {
        int partIdx;
        if (TCL_OK == Tcl_GetIntFromObj(interp, objv[argc], &partIdx))
        {
            if ((size_t)partIdx >= swPart->m_hwPartList.size())
            {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: hwPart index out of range", -1));
                return TCL_ERROR;
            }
            const Fantom::Part *part = swPart->m_hwPartList[partIdx];
            return parseGetHwPart(interp, argc+1, objc, objv, part);
        }
        else if (strcmp(Tcl_GetStringFromObj(objv[argc], 0), "max") == 0)
        {
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)swPart->m_hwPartList.size()));
            return TCL_OK;
        }
        else
        {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid hwPart index", -1));
            return TCL_ERROR;
        }
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing hwPart index", -1));
    return TCL_ERROR;
}

int parseGetHwPartList(Tcl_Interp *interp, int argc, int objc, Tcl_Obj *const objv[], const Fantom::Performance *performance)
{
    if (argc < objc)
    {
        int partIdx;
        if (TCL_OK == Tcl_GetIntFromObj(interp, objv[argc], &partIdx))
        {
            if ((size_t)partIdx >= performance->NofParts)
            {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: part index out of range", -1));
                return TCL_ERROR;
            }
            const Fantom::Part *part = &performance->m_partList[partIdx];
            return parseGetHwPart(interp, argc+1, objc, objv, part);
        }
        else if (strcmp(Tcl_GetStringFromObj(objv[argc], 0), "max") == 0)
        {
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)performance->NofParts));
            return TCL_OK;
        }
        else
        {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid part index", -1));
            return TCL_ERROR;
        }
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing part index", -1));
    return TCL_ERROR;
}

int parseGetPerformance(Tcl_Interp *interp, int argc, int objc, Tcl_Obj *const objv[], const Track *track)
{
    if (argc < objc)
    {
        const Fantom::Performance *performance = track->m_performance;
        const char *member = Tcl_GetStringFromObj(objv[argc], 0);
        if (strcmp(member, "name") == 0)
            Tcl_SetObjResult(interp, Tcl_NewStringObj(performance->m_name, -1));
        else if (strcmp(member, "part") == 0)
            return parseGetHwPartList(interp, ++argc, objc, objv, performance);
        else
        {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid performance field", -1));
            return TCL_ERROR;
        }
        return TCL_OK;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing performance field", -1));
    return TCL_ERROR;
}

int parseGetSwPart(Tcl_Interp *interp, int argc, int objc, Tcl_Obj *const objv[], const Section *section)
{
    if (argc < objc)
    {
        int partIdx;
        if (TCL_OK == Tcl_GetIntFromObj(interp, objv[argc], &partIdx))
        {
            if ((size_t)partIdx >= section->m_partList.size())
            {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: part index out of range", -1));
                return TCL_ERROR;
            }
            const SwPart *part = section->m_partList[partIdx];
            argc++;
            if (argc < objc)
            {
                const char *member = Tcl_GetStringFromObj(objv[argc], 0);
                if (strcmp(member, "number") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewIntObj(part->m_number));
                else if (strcmp(member, "name") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewStringObj(part->m_name, -1));
                else if (strcmp(member, "channel") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_channel));
                else if (strcmp(member, "transpose") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_transpose));
                else if (strcmp(member, "customTransposeEnabled") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewBooleanObj((int)part->m_customTransposeEnabled));
                else if (strcmp(member, "rangeLower") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_rangeLower));
                else if (strcmp(member, "rangeUpper") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_rangeUpper));
                else if (strcmp(member, "mono") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewBooleanObj((int)part->m_mono));
                else if (strcmp(member, "hwPart") == 0)
                    return parseGetHwPartList(interp, argc+1, objc, objv, part);
                else
                {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid part field", -1));
                    return TCL_ERROR;
                }
                return TCL_OK;
            }
            Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing part field", -1));
            return TCL_ERROR;
        }
        else if (strcmp(Tcl_GetStringFromObj(objv[argc], 0), "max") == 0)
        {
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)section->m_partList.size()));
            return TCL_OK;
        }
        else
        {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid part index", -1));
            return TCL_ERROR;
        }
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing part index", -1));
    return TCL_ERROR;
}

int parseGetSection(Tcl_Interp *interp, int argc, int objc, Tcl_Obj *const objv[], const Track *track)
{
    if (argc < objc)
    {
        int sectionIdx;
        if (TCL_OK == Tcl_GetIntFromObj(interp, objv[argc], &sectionIdx))
        {
            if ((size_t)sectionIdx >= track->m_sectionList.size())
            {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: section index out of range", -1));
                return TCL_ERROR;
            }
            const Section *section = track->m_sectionList[sectionIdx];
            argc++;
            if (argc < objc)
            {
                const char *member = Tcl_GetStringFromObj(objv[argc], 0);
                if (strcmp(member, "name") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewStringObj(section->m_name, -1));
                else if (strcmp(member, "noteOffEnter") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(section->m_noteOffEnter));
                else if (strcmp(member, "noteOffLeave") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(section->m_noteOffLeave));
                else if (strcmp(member, "nextTrack") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewIntObj(section->m_nextTrack));
                else if (strcmp(member, "nextSection") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewIntObj(section->m_nextSection));
                else if (strcmp(member, "previousTrack") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewIntObj(section->m_previousTrack));
                else if (strcmp(member, "previousSection") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewIntObj(section->m_previousSection));
                else if (strcmp(member, "part") == 0)
                    return parseGetSwPart(interp, ++argc, objc, objv, section);
                else
                {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid section field", -1));
                    return TCL_ERROR;
                }
                return TCL_OK;
            }
            Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing section field", -1));
            return TCL_ERROR;
        }
        else if (strcmp(Tcl_GetStringFromObj(objv[argc], 0), "max") == 0)
        {
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)track->m_sectionList.size()));
            return TCL_OK;
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid section index", -1));
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing section index", -1));
    return TCL_ERROR;
}

int parseGetTrack(Tcl_Interp *interp, int argc, int objc, Tcl_Obj *const objv[])
{
    if (argc < objc)
    {
        int trackIdx;
        if (TCL_OK == Tcl_GetIntFromObj(interp, objv[argc], &trackIdx))
        {
            if ((size_t)trackIdx >= tkClientState.m_trackList.size())
            {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: track index out of range", -1));
                return TCL_ERROR;
            }
            const Track *track = tkClientState.m_trackList[trackIdx];
            argc++;
            if (argc < objc)
            {
                const char *member = Tcl_GetStringFromObj(objv[argc], 0);
                if (strcmp(member, "name") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewStringObj(track->m_name, -1));
                else if (strcmp(member, "chain") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(track->m_chain));
                else if (strcmp(member, "startSection") == 0)
                    Tcl_SetObjResult(interp, Tcl_NewIntObj(track->m_startSection));
                else if (strcmp(member, "section") == 0)
                    return parseGetSection(interp, ++argc, objc, objv, track);
                else if (strcmp(member, "performance") == 0)
                    return parseGetPerformance(interp, ++argc, objc, objv, track);
                else
                {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid track field", -1));
                    return TCL_ERROR;
                }
                return TCL_OK;
            }
            Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing track field", -1));
            return TCL_ERROR;
        }
        else if (strcmp(Tcl_GetStringFromObj(objv[argc], 0), "max") == 0)
        {
            Tcl_SetObjResult(interp, Tcl_NewIntObj((int)tkClientState.m_trackList.size()));
            return TCL_OK;
        }
        Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid track index", -1));
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing track index", -1));
    return TCL_ERROR;
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

int xmlGet(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    (void)cdata;
    int argc = 1;
    if (argc < objc)
    {
        if (strcmp(Tcl_GetStringFromObj(objv[argc], 0), "track") == 0)
            return parseGetTrack(interp, argc+1, objc, objv);
        else if (strcmp(Tcl_GetStringFromObj(objv[argc], 0), "set") == 0)
            return getSet(interp);
        else
        {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: invalid argument", -1));
            return TCL_ERROR;
        }
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj("xmlget: missing argument", -1));
    return TCL_ERROR;
}

int processSectionChange(Tcl_Interp *interp, uint8_t newSection)
{
    tkClientState.m_currentSection = newSection;
    TclEval eval;
    eval.stream() << ".currentFrame.section configure -text {section " << (tkClientState.m_currentSection+1)
                  << "/" << tkClientState.currentTrack()->m_sectionList.size() << " "
                  << tkClientState.currentSection()->m_name << "}";
    eval.flush(interp);
    int num = 0;
    for (int i=0; i<12 && tkClientState.m_swPartState[i]; i++)
    {
        tkClientState.m_swPartState[i]->clear(interp);
    }
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
    if (tkClientState.m_currentSection >= 0)
    {
        TclEval eval;
        eval.stream() << ".currentFrame.track configure -text {track " << (tkClientState.m_currentTrack+1)
                      << "/" << tkClientState.m_trackList.size() << " "
                      << tkClientState.currentTrack()->m_name << "}";
        eval.flush(interp);
    }
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
    Tcl_CreateObjCommand(interp, "xmlget", xmlGet, NULL, NULL);
    Tcl_CreateObjCommand(interp, "processEvent", processEvent, NULL, NULL);
    return TCL_OK;
}
} // extern "C"
