/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#ifndef ASYLUM_VIDEO_H
#define ASYLUM_VIDEO_H

#include "asylum/shared.h"

#include "common/array.h"
#include "common/events.h"
#include "common/system.h"
#include "common/list.h"

#include "graphics/surface.h"
#include "graphics/video/smk_decoder.h"

#include "sound/mixer.h"

namespace Asylum {

class AsylumEngine;
class GraphicResource;
class VideoText;
struct GraphicFrame;

struct VideoSubtitle {
	int frameStart;
	int frameEnd;
	ResourceId textResourceId;
};

class Video {
public:
	Video(AsylumEngine *engine, Audio::Mixer *mixer);
	virtual ~Video();

	void playVideo(int32 videoNumber);

private:
	void performPostProcessing(byte *screen);
	void loadSubtitles(int32 videoNumber);
	void processVideoEvents();

	bool _skipVideo;
	VideoText *_text;
	Graphics::SmackerDecoder *_smkDecoder;
	Common::List<Common::Event> _stopEvents;
	Common::Array<VideoSubtitle> _subtitles;
}; // end of class Video

// The VideoText class has some methods from the Text class,
// but it differs from the text class: this class draws text
// to a predefined screen buffer, whereas the Text class draws
// text directly to the screen
class VideoText {
public:
	VideoText(AsylumEngine *engine);
	~VideoText();

	void loadFont(ResourceId resourceId);
	void drawMovieSubtitle(byte *screenBuffer, ResourceId resourceId);

private:
	AsylumEngine *_vm;

	int32 getTextWidth(const char *text);

	void drawText(byte *screenBuffer, int16 x, int16 y, const char *text);
	void copyToVideoFrame(byte *screenBuffer, GraphicFrame *frame, int x, int y) const;

	GraphicResource *_fontResource;
	uint8           _curFontFlags;

}; // end of class VideoText

} // end of namespace Asylum

#endif