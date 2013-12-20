#include <tcl.h>
#include <string.h>
#include "trackdef.h"
#include "fantomdef.h"
#include "xml.h"

class TkClientState
{
public:
    TrackList m_trackList;
    SetList m_setList;
};

TkClientState tkClientState;

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

int parseGetHwPart(Tcl_Interp *interp, int argc, int objc, Tcl_Obj *const objv[], const Fantom::Performance *performance)
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
            argc++;
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
            return parseGetHwPart(interp, ++argc, objc, objv, performance);
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
                    Tcl_SetObjResult(interp, Tcl_NewIntObj((int)part->m_number));
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
                // \todo complete member list
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
    Tcl_CreateObjCommand(interp, "loadxml", loadXml, NULL, NULL);
    Tcl_CreateObjCommand(interp, "xmlget", xmlGet, NULL, NULL);
    return TCL_OK;
}
} // extern "C"
