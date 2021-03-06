/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */




#include "Common.h"

#include "HostWindowDataSoftware.h"

#include <QImage>
#include <PIpcBuffer.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include "Logging.h"
#include "WindowMetaData.h"
#include "WebAppMgrProxy.h"

HostWindowDataSoftware::HostWindowDataSoftware(int key, int metaDataKey, int width, int height, bool hasAlpha)
	: m_ipcBuffer(0)
	, m_metaDataBuffer(0)
	, m_transitionBuffer(0)
	, m_transitionPixmap(0)
	, m_width(width)
	, m_height(height)
	, m_hasAlpha(hasAlpha)
	, m_dirty(false)
{
	m_ipcBuffer = PIpcBuffer::attach(key);
	if (!m_ipcBuffer) {
		g_critical("%s (%d): Failed to attach to shared buffer with key: %d",
				   __PRETTY_FUNCTION__, __LINE__, key);
	}

	m_metaDataBuffer = PIpcBuffer::attach(metaDataKey);
	if (!m_metaDataBuffer) {
		g_critical("%s (%d): Failed to attach to metadata shared buffer with key: %d",
				   __PRETTY_FUNCTION__, __LINE__, metaDataKey);
	}
}

HostWindowDataSoftware::~HostWindowDataSoftware()
{
	delete m_ipcBuffer;
	delete m_transitionPixmap;
	delete m_transitionBuffer;
	delete m_metaDataBuffer;
}

void HostWindowDataSoftware::flip()
{
	int width = m_width;
	m_width = m_height;
	m_height = width;

	if (m_transitionPixmap && m_transitionBuffer) {
		delete m_transitionPixmap;
		QImage img = QImage((const uchar*) m_transitionBuffer->data(), m_width, m_height, QImage::Format_ARGB32_Premultiplied);
		m_transitionPixmap = new QPixmap(QPixmap::fromImage(img));
	}
}

void HostWindowDataSoftware::onUpdateRegion(QPixmap& screenPixmap, int x, int y, int w, int h)
{
	m_dirty = true;
}

QPixmap* HostWindowDataSoftware::acquirePixmap(QPixmap& screenPixmap)
{
	if (m_dirty) {

	    if (G_UNLIKELY(!m_ipcBuffer))
		return &screenPixmap;

		m_ipcBuffer->lock();
	    QImage sharedImage = QImage((const uchar*) m_ipcBuffer->data(), m_width, m_height,
					QImage::Format_ARGB32_Premultiplied);
	    sharedImage.detach();
	    screenPixmap = QPixmap::fromImage(sharedImage);
		m_ipcBuffer->unlock();
	}

	return &screenPixmap;
}

void HostWindowDataSoftware::onUpdateWindowRequest()
{
	// NO-OP    
}

void HostWindowDataSoftware::onSceneTransitionPrepare(int width, int height)
{
	if (!m_ipcBuffer) {
		g_critical("%s (%d): Failed to get remote window IPC buffer", __PRETTY_FUNCTION__, __LINE__);
		return;
	}

	WindowMetaData* metaData = (WindowMetaData*) m_metaDataBuffer->data();
	m_metaDataBuffer->lock();
	int transKey = metaData->transitionBufferKey;
	m_metaDataBuffer->unlock();
		
	if (0 == transKey) {
		g_critical("%s (%d): Failed to get IPC key for the shared transition sufrace", __PRETTY_FUNCTION__, __LINE__);
		return;
	}

	PIpcBuffer* transitionBuffer = PIpcBuffer::attach(transKey);

	setIpcTransitionBuffer(transitionBuffer);

	if (!transitionBuffer) {
		g_critical("%s (%d): Failed to attach to shared transition buffer with key: %d", __PRETTY_FUNCTION__, __LINE__,
				transKey);
		return;
	}

	g_message("%s (%d): Attached to shared transition buffer with key: %d, width: %d, height: %d", __PRETTY_FUNCTION__,
			__LINE__, transKey, width, height);

	QImage img = QImage((const uchar*) transitionBuffer->data(), width, height, QImage::Format_ARGB32_Premultiplied);
	m_transitionPixmap = new QPixmap(QPixmap::fromImage(img));
}

void HostWindowDataSoftware::onSceneTransitionFinish()
{
	if (m_transitionPixmap) {
		delete m_transitionPixmap;
		m_transitionPixmap = NULL;
	}

	if (m_transitionBuffer) {
		delete m_transitionBuffer;
		m_transitionBuffer = NULL;
	}
}

void HostWindowDataSoftware::setIpcTransitionBuffer(PIpcBuffer* buffer)
{
	delete m_transitionPixmap;
	m_transitionPixmap = 0;
	delete m_transitionBuffer;
	m_transitionBuffer = buffer;
}

void HostWindowDataSoftware::updateFromAppDirectRenderingLayer(int screenX, int screenY, int screenOrientation)
{
	// FIXME    
}
