/*!
 * COPYRIGHT (C) 2011 Emeric Grange - All Rights Reserved
 *
 * This file is part of MiniVideo.
 *
 * MiniVideo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MiniVideo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with MiniVideo.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \file      mkv_struct.h
 * \author    Emeric Grange <emeric.grange@gmail.com>
 * \date      2011
 */

#ifndef PARSER_MKV_STRUCT_H
#define PARSER_MKV_STRUCT_H

// minivideo headers
#include "../../minivideo_typedef.h"
#include "../../minivideo_codecs.h"
#include "../../thirdparty/simple_vector.h"

#include <stdio.h>

/* ************************************************************************** */

typedef struct ebml_header_t
{
    uint64_t EBMLVersion;
    uint64_t EBMLReadVersion;
    uint64_t EBMLMaxIDLength;
    uint64_t EBMLMaxSizeLength;
    char *DocType;
    uint64_t DocTypeVersion;
    uint64_t DocTypeReadVersion;

} ebml_header_t;

typedef struct mkv_info_chapter_t
{
    uint64_t ChapterTranslateEditionUID;
    uint64_t ChapterTranslateCodec;
    uint8_t *ChapterTranslateID;

} mkv_info_chapter_t;

typedef struct mkv_info_t
{
    uint8_t *SegmentUID;
    char *SegmentFilename;
    uint8_t *PrevUID;
    char *PrevFilename;
    uint8_t *NextUID;
    char *NextFilename;
    uint8_t *SegmentFamily;

    int chapter_count;
    mkv_info_chapter_t *chapter;

    uint64_t TimecodeScale;
    double Duration;
    uint64_t DateUTC;
    char *Title;
    char *MuxingApp;
    char *WritingApp;

} mkv_info_t;

typedef struct mkv_cluster_t
{
    uint64_t Timecode;
    //SilentTracks
    //SilentTrackNumber
    uint64_t Position;
    uint64_t PrevSize;
    //uint8_t *SimpleBlock;
    //BlockGroup;
    //Block;

} mkv_cluster_t;

typedef struct mkv_tag_t
{
    uint64_t CueTrack;
    uint64_t CueClusterPosition;
    uint64_t CueRelativePosition;
    uint64_t CueDuration;
    uint64_t CueBlockNumber;
    uint64_t CueCodecState;

} mkv_tag_t;

typedef struct mkv_tags_t
{
    mkv_tag_t *Tags;

} mkv_tags_t;

typedef struct mkv_cuetrackpos_t
{
    uint64_t CueTrack;
    uint64_t CueClusterPosition;
    uint64_t CueRelativePosition;
    uint64_t CueDuration;
    uint64_t CueBlockNumber;
    uint64_t CueCodecState;

} mkv_cuetrackpos_t;

typedef struct mkv_cuepoint_t
{
    uint64_t CueTime;
    mkv_cuetrackpos_t *CueTrackPos;

} mkv_cuepoint_t;

typedef struct mkv_attachedfile_t
{
    char *FileDescription;
    char *FileName;
    char *FileMimeType;
    uint64_t FileData_size;
    uint8_t *FileData;
    uint64_t FileUID;

} mkv_attachedfile_t;

typedef struct mkv_attachments_t
{
    mkv_attachedfile_t *file;

} mkv_attachments_t;

/* ************************************************************************** */

typedef struct mkv_sample_t
{
    int64_t offset;
    int64_t size;
    int64_t timecode;

    bool idr;
    bool visible;
    bool discardable;

} mkv_sample_t;

/* ************************************************************************** */

typedef struct mkv_track_audio_t
{
    double SamplingFrequency;
    double OutputSamplingFrequency;
    uint64_t Channels;
    uint64_t BitDepth;

} mkv_track_audio_t;

typedef struct mkv_track_video_colour_mastering_t
{
    double PrimaryRChromaticityX;
    double PrimaryRChromaticityY;
    double PrimaryGChromaticityX;
    double PrimaryGChromaticityY;
    double PrimaryBChromaticityX;
    double PrimaryBChromaticityY;
    double WhitePointChromaticityX;
    double WhitePointChromaticityY;
    double LuminanceMax;
    double LuminanceMin;

} mkv_track_video_colour_mastering_t;

typedef struct mkv_track_video_colour_t
{
    uint64_t MatrixCoefficients;
    uint64_t BitsPerChannel;
    uint64_t ChromaSubsamplingHorz;
    uint64_t ChromaSubsamplingVert;
    uint64_t CbSubsamplingHorz;
    uint64_t CbSubsamplingVert;
    uint64_t ChromaSitingHorz;
    uint64_t ChromaSitingVert;
    uint64_t Range;
    uint64_t TransferCharacteristics;
    uint64_t Primaries;
    uint64_t MaxCLL;
    uint64_t MaxFALL;
    mkv_track_video_colour_mastering_t *MasteringMetadata;

} mkv_track_video_colour_t;

typedef struct mkv_track_video_t
{
    uint64_t FlagInterlaced;
    uint64_t FieldOrder;
    uint64_t StereoMode;
    uint64_t AlphaMode;
    uint64_t PixelWidth;
    uint64_t PixelHeight;
    uint64_t PixelCropBottom;
    uint64_t PixelCropTop;
    uint64_t PixelCropLeft;
    uint64_t PixelCropRight;
    uint64_t DisplayWidth;
    uint64_t DisplayHeight;
    uint64_t DisplayUnit;
    uint64_t AspectRatioType;
    int ColourSpace_size;
    uint8_t *ColourSpace;
    mkv_track_video_colour_t *Colour;

} mkv_track_video_t;

typedef struct mkv_track_translate_t
{
    uint64_t todo; // TODO

} mkv_track_translate_t;

typedef struct mkv_track_operation_t
{
    uint64_t todo; // TODO

} mkv_track_operation_t;

typedef struct mkv_track_encoding_t
{
    uint64_t ContentEncodingOrder;
    uint64_t ContentEncodingScope;
    uint64_t ContentEncodingType;

        uint64_t ContentCompAlgo;
        int ContentCompSettings_size;
        uint8_t *ContentCompSettings;

        uint64_t ContentEncAlgo;
        int ContentEncKeyID_size;
        uint8_t *ContentEncKeyID;
        int ContentSignature_size;
        uint8_t *ContentSignature;
        int ContentSigKeyID_size;
        uint8_t *ContentSigKeyID;
        uint64_t ContentSigAlgo;
        uint64_t ContentSigHashAlgo;

} mkv_track_encoding_t;

typedef struct mkv_track_encodings_t
{
    mkv_track_encoding_t *encoding;

} mkv_track_encodings_t;

typedef struct mkv_track_t
{
    uint64_t TrackNumber;
    uint64_t TrackUID;
    uint64_t TrackType;
    uint64_t FlagEnabled;
    uint64_t FlagDefault;
    uint64_t FlagForced;
    uint64_t FlagLacing;
    uint64_t MinCache;
    uint64_t MaxCache;
    uint64_t DefaultDuration;
    uint64_t DefaultDecodedFieldDuration;
    uint64_t TrackTimecodeScale;    //!< DEPRECATED
    uint64_t MaxBlockAdditionID;
    char *Name;
    char *Language;
    char *CodecID;
    int CodecPrivate_size;
    uint8_t *CodecPrivate;
    char *CodecName;
    uint64_t AttachmentLink;
    uint64_t CodecDecodeAll;
    uint64_t TrackOverlay;
    uint64_t CodecDelay;
    uint64_t SeekPreRoll;

    mkv_track_audio_t *audio;
    mkv_track_video_t *video;
    mkv_track_translate_t *translate;
    mkv_track_operation_t *operation;
    mkv_track_encodings_t *encodings;

    vector sample_vector;

} mkv_track_t;

/* ************************************************************************** */

//! Structure for MKV video infos
typedef struct mkv_t
{
    bool run;                   //!< A convenient way to stop the parser from any sublevel

    ContainerProfiles_e profile;//!< MKV variant

    ebml_header_t ebml;
    mkv_info_t info;

    int tracks_count;
    mkv_track_t *tracks[16];

    FILE *xml;                  //!< Temporary file used by the xmlMapper

} mkv_t;

/* ************************************************************************** */

/*!
 * \enum MkvTrackType_e
 * \brief Identifies the content of a track.
 */
typedef enum MkvTrackType_e
{
    MKV_TRACK_VIDEO     = 1,
    MKV_TRACK_AUDIO     = 2,
    MKV_TRACK_COMPLEX   = 3,
    MKV_TRACK_LOGO      = 0x10,
    MKV_TRACK_SUBTITLES = 0x11,
    MKV_TRACK_BUTTONS   = 0x12,
    MKV_TRACK_CONTROL   = 0x20,

} Mp4HandlerType_e;

/*!
 * \enum EbmlDocType_e
 * \brief Identifies the doctype of the current EBML file.
 */
typedef enum EbmlDocType_e
{
    doctype_unknown  = 0,

    doctype_matroska = 1,
    doctype_webm     = 2

} EbmlDocType_e;

/*!
 * \enum EbmlElement_e
 * \brief Identifies the EBML element type.
 *
 * The element marked with an * are mandatory.
 */
typedef enum EbmlElement_e
{
    eid_EBML = 0x1A45DFA3,              //!< * (level 0) EBML file header
        eid_EBMLVersion = 0x4286,       //!<
        eid_EBMLReadVersion = 0x42F7,   //!<
        eid_EBMLMaxIDLength = 0x42F2,   //!<
        eid_EBMLMaxSizeLength = 0x42F3, //!<
        eid_DocType = 0x4282,           //!<
        eid_DocTypeVersion = 0x4287,    //!<
        eid_DocTypeReadVersion = 0x4285,//!<

    eid_Segment = 0x18538067,           //!< * (level 0) This element contains all other top-level (level 1) elements.

    eid_SeekHead = 0x114D9B74,          //!< (level 1) Meta Seek Information
        eid_Seek = 0x4DBB,              //!<
            eid_SeekId = 0x53AB,        //!<
            eid_SeekPosition = 0x53AC,  //!<

    eid_Info = 0x1549A966,              //!< * (level 1) Segment Information
        eid_SegmentUID = 0x73A4,        //!<
        eid_SegmentFilename = 0x7384,   //!<
        eid_PrevUID = 0x3CB923,         //!<
        eid_PrevFilename = 0x3C83AB,    //!<
        eid_NextUID = 0x3EB923,         //!<
        eid_NextFilename = 0x3E83BB,    //!<
        eid_SegmentFamily = 0x4444,     //!<
        eid_ChapterTranslate = 0x6924,  //!<
            eid_ChapterTranslateEditionUID = 0x69FC,
            eid_ChapterTranslateCodec = 0x69BF,
            eid_ChapterTranslateID = 0x69A5,
        eid_TimecodeScale = 0x2AD7B1,   //!<
        eid_Duration = 0x4489,          //!<
        eid_DateUTC = 0x4461,           //!<
        eid_Title = 0x7BA9,             //!<
        eid_MuxingApp = 0x4D80,         //!<
        eid_WritingApp = 0x5741,        //!<

    eid_Cluster = 0x1F43B675,           //!< (level 1) Cluster
        eid_TimeCode = 0xE7,
        eid_SilentTracks = 0x5854,
            eid_SilentTrackNumber = 0x58D7,
        eid_Position = 0xA7,
        eid_PrevSize = 0xAB,
        eid_SimpleBlock = 0xA3,         //!<
        eid_BlockGroup = 0xA0,
        eid_Block = 0xA1,               //!<
            eid_BlockAdditions = 0x75A1,
                eid_BlockMore = 0xA6,
                    eid_BlockAddID = 0xEE,
                    eid_BlockAdditional = 0xA5,
            eid_BlockDuration = 0x9B,
            eid_ReferencePriority = 0xFA,
            eid_ReferenceBlock = 0xFB,
            eid_CodecState = 0xA4,
            eid_DiscardPadding = 0x75A2,
            eid_Slices = 0x8E,
                eid_TimeSlice = 0xE8,
                    eid_LaceNumber = 0xCC,

    eid_Tracks = 0x1654AE6B,            //!< (level 1) Track
    eid_TrackEntry = 0xAE,              //!<
        eid_TrackNumber = 0xD7,
        eid_TrackUID = 0x73C5,
        eid_TrackType = 0x83,
        eid_FlagEnabled = 0xB9,
        eid_FlagDefault = 0x88,
        eid_FlagForced = 0x55AA,
        eid_FlagLacing = 0x9C,
        eid_MinCache = 0x6DE7,
        eid_MaxCache = 0x6DF8,
        eid_DefaultDuration = 0x23E383,
        eid_DefaultDecodedFieldDuration = 0x234E7A,
        eid_TrackTimecodeScale = 0x23314F,  //!< DEPRECATED
        eid_MaxBlockAdditionID = 0x55EE,
        eid_Name = 0x536E,
        eid_Language = 0x22B59C,
        eid_CodecID = 0x86,
        eid_CodecPrivate = 0x63A2,
        eid_CodecName = 0x258688,
        eid_AttachmentLink = 0x7446,
        eid_CodecDecodeAll = 0xAA,
        eid_TrackOverlay = 0x6FAB,
        eid_CodecDelay = 0x56AA,
        eid_SeekPreRoll = 0x56BB,
    eid_TrackTranslate = 0x6624,        //!<
        eid_TrackTranslateEditionUID = 0x66FC,
        eid_TrackTranslateCodec = 0x66BF,
        eid_TrackTranslateTrackID = 0x66A5,
    eid_Video = 0xE0,                   //!<
        eid_FlagInterlaced = 0x9A,
        eid_FieldOrder = 0x9D,
        eid_StereoMode = 0x53B8,
        eid_AlphaMode = 0x53C0,
        eid_PixelWidth = 0xB0,
        eid_PixelHeight = 0xBA,
        eid_PixelCropBottom = 0x54AA,
        eid_PixelCropTop = 0x54BB,
        eid_PixelCropLeft = 0x54CC,
        eid_PixelCropRight = 0x54DD,
        eid_DisplayWidth = 0x54B0,
        eid_DisplayHeight = 0x54BA,
        eid_DisplayUnit = 0x54B2,
        eid_AspectRatioType = 0x54B3,
        eid_ColourSpace = 0x2EB524,
        eid_Colour = 0x55B0,        //!<
            eid_MatrixCoefficients = 0x55B1,
            eid_BitsPerChannel = 0x55B2,
            eid_ChromaSubsamplingHorz = 0x55B3,
            eid_ChromaSubsamplingVert = 0x55B4,
            eid_CbSubsamplingHorz = 0x55B5,
            eid_CbSubsamplingVert = 0x55B6,
            eid_ChromaSitingHorz = 0x55B7,
            eid_ChromaSitingVert = 0x55B8,
            eid_Range = 0x55B9,
            eid_TransferCharacteristics = 0x55BA,
            eid_Primaries =  0x55BB,
            eid_MaxCLL = 0x55BC,
            eid_MaxFALL = 0x55BD,
            eid_MasteringMetadata = 0x55D0,
                eid_PrimaryRChromaticityX = 0x55D1,
                eid_PrimaryRChromaticityY = 0x55D2,
                eid_PrimaryGChromaticityX = 0x55D3,
                eid_PrimaryGChromaticityY = 0x55D4,
                eid_PrimaryBChromaticityX = 0x55D5,
                eid_PrimaryBChromaticityY = 0x55D6,
                eid_WhitePointChromaticityX = 0x55D7,
                eid_WhitePointChromaticityY = 0x55D8,
                eid_LuminanceMax = 0x55D9,
                eid_LuminanceMin = 0x55DA,
    eid_Audio = 0xE1,                   //!<
        eid_SamplingFrequency = 0xB5,
        eid_OutputSamplingFrequency = 0x78B5,
        eid_Channels = 0x9F,
        eid_ChannelPositions = 0x7D7B,
        eid_BitDepth = 0x6264,
    eid_TrackOperation = 0xE2,          //!<
        eid_TrackCombinePlanes = 0xE3,
        eid_TrackPlane = 0xE4,
        eid_TrackPlaneUID = 0xE5,
        eid_TrackPlaneType = 0xE6,
        eid_TrackJoinBlocks = 0xE9,
            eid_TrackJoinUID = 0xED,
    eid_ContentEncodings = 0x6D80,      //!<
        eid_ContentEncoding = 0x6240,
            eid_ContentEncodingOrder = 0x5031,
            eid_ContentEncodingScope = 0x5032,
            eid_ContentEncodingType = 0x5033,
            eid_ContentCompression = 0x5034,
                eid_ContentCompAlgo = 0x4254,
                eid_ContentCompSettings = 0x4255,
            eid_ContentEncryption = 0x5035,
                eid_ContentEncAlgo = 0x47E1,
                eid_ContentEncKeyID = 0x47E2,
                eid_ContentSignature = 0x47E3,
                eid_ContentSigKeyID = 0x47E4,
                eid_ContentSigAlgo = 0x47E5,
                eid_ContentSigHashAlgo = 0x47E6,

    eid_Cues = 0x1C53BB6B,              //!< (level 1) Cueing Data
        eid_CuePoint = 0xBB,            //!< Contains all information relative to a seek point in the Segment
            eid_CueTime = 0xB3,
            eid_CueTrackPositions = 0xB7,
                eid_CueTrack = 0xF7,
                eid_CueClusterPosition = 0xF1,
                eid_CueRelativePosition = 0xF0,
                eid_CueDuration = 0xB2,
                eid_CueBlockNumber = 0x5378,
                eid_CueCodecState = 0xEA,
                eid_CueReference = 0xDB,
                eid_CueRefTime = 0x96,

    eid_Attachments = 0x1941A469,       //!< (level 1) Attachment
        eid_AttachedFile = 0x61A7,      //!< An attached file
            eid_FileDescription = 0x467E,
            eid_FileName = 0x466E,
            eid_FileMimeType = 0x4660,
            eid_FileData = 0x465C,
            eid_FileUID = 0x46AE,

    eid_Chapters = 0x1043A770,          //!< (level 1) Chapter
        eid_EditionEntry = 0x45B9,
            eid_EditionUID = 0x45BC,
            eid_EditionFlagHidden = 0x45BD,
            eid_EditionFlagDefault = 0x45DB,
            eid_EditionFlagOrdered = 0x45DD,
            eid_ChapterAtom = 0xB6,
                eid_ChapterUID = 0x73C4,
                eid_ChapterStringUID = 0x5654,
                eid_ChapterTimeStart = 0x91,
                eid_ChapterTimeEnd = 0x92,
                eid_ChapterFlagHidden = 0x98,
                eid_ChapterFlagEnabled = 0x4598,
                eid_ChapterSegmentUID = 0x6E67,
                eid_ChapterSegmentEditionUID = 0x6EBC,
                eid_ChapterPhysicalEquiv = 0x63C3,
                eid_ChapterTrack = 0x8F,
                    eid_ChapterTrackNumber = 0x89,
                eid_ChapterDisplay = 0x80,
                    eid_ChapString = 0x85,
                    eid_ChapLanguage = 0x437C,
                    eid_ChapCountry = 0x437E,
                eid_ChapProcess = 0x6944,
                    eid_ChapProcessCodecID = 0x6955,
                    eid_ChapProcessPrivate = 0x450D,
                    eid_ChapProcessCommand = 0x6911,
                        eid_ChapProcessTime = 0x6922,
                        eid_ChapProcessData = 0x6933,

    eid_Tags = 0x1254C367,              //!< (level 1) Tagging
        eid_Tag = 0x7373,
            eid_Targets = 0x63C0,
                eid_TargetTypeValue = 0x68CA,
                eid_TargetType = 0x63CA,
                eid_TagTrackUID = 0x63C5,
                eid_TagEditionUID = 0x63C9,
                eid_TagChapterUID = 0x63C4,
                eid_TagAttachmentUID = 0x63C6,
            eid_SimpleTag = 0x67C8,
                eid_TagName = 0x45A3,
                eid_TagLanguage = 0x447A,
                eid_TagDefault = 0x4484,
                eid_TagString = 0x4487,
                eid_TagBinary = 0x4485,

    eid_void = 0xEC,                    //!< (global)
    eid_crc32 = 0xBF                    //!< (global)

} EbmlElement_e;

/* ************************************************************************** */
#endif // PARSER_MKV_STRUCT_H
