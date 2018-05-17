// +-------------------------------------------------------------------------
// | Copyright (C) 2017 Yunify, Inc.
// +-------------------------------------------------------------------------
// | Licensed under the Apache License, Version 2.0 (the "License");
// | You may not use this work except in compliance with the License.
// | You may obtain a copy of the License in the LICENSE file, or at:
// |
// | http://www.apache.org/licenses/LICENSE-2.0
// |
// | Unless required by applicable law or agreed to in writing, software
// | distributed under the License is distributed on an "AS IS" BASIS,
// | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// | See the License for the specific language governing permissions and
// | limitations under the License.
// +-------------------------------------------------------------------------

#include "data/Cache.h"

#include <assert.h>
#include <string.h>

#include <algorithm>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "boost/exception/to_string.hpp"
#include "boost/make_shared.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/thread/locks.hpp"
#include "boost/tuple/tuple.hpp"

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/TimeUtils.h"
#include "base/Utils.h"
#include "base/UtilsWithLog.h"
#include "configure/Options.h"
#include "data/DirectoryTree.h"
#include "data/File.h"
#include "data/Node.h"
#include "data/Page.h"
#include "data/StreamUtils.h"

namespace QS {

namespace Data {

using boost::lock_guard;
using boost::make_shared;
using boost::recursive_mutex;
using boost::shared_ptr;
using boost::to_string;
using boost::tuple;
using QS::Data::StreamUtils::GetStreamSize;
using QS::StringUtils::FormatPath;
using QS::StringUtils::PointerAddress;
using QS::TimeUtils::SecondsToRFC822GMT;
using QS::UtilsWithLog::CreateDirectoryIfNotExists;
using QS::UtilsWithLog::IsSafeDiskSpace;
using std::iostream;
using std::list;
using std::make_pair;
using std::pair;
using std::string;
using std::vector;

// --------------------------------------------------------------------------
bool Cache::HasFreeSpace(size_t size) const {
  return GetSize() + size <= GetCapacity();
}

// --------------------------------------------------------------------------
bool Cache::IsLastFileOpen() const {
  lock_guard<recursive_mutex> locker(m_mutex);
  if (m_cache.empty()) {
    return false;
  }
  return m_cache.back().second->IsOpen();
}

// --------------------------------------------------------------------------
bool Cache::HasFile(const string &filePath) const {
  lock_guard<recursive_mutex> locker(m_mutex);
  return m_map.find(filePath) != m_map.end();
}

// --------------------------------------------------------------------------
size_t Cache::GetNumFile() const {
  lock_guard<recursive_mutex> locker(m_mutex);
  return m_map.size();
}

// --------------------------------------------------------------------------
uint64_t Cache::GetFileSize(const std::string &filePath) const {
  lock_guard<recursive_mutex> locker(m_mutex);
  CacheMapConstIterator it = m_map.find(filePath);
  if (it != m_map.end()) {
    shared_ptr<File> &file = it->second->second;
    return file->GetSize();
  } else {
    // file is not in cache, return 0
    return 0;
  }
}

// --------------------------------------------------------------------------
shared_ptr<File> Cache::FindFile(const string &filePath) {
  lock_guard<recursive_mutex> locker(m_mutex);
  CacheMapIterator it = m_map.find(filePath);
  CacheListIterator pos;
  if (it != m_map.end()) {
    pos = UnguardedMakeFileMostRecentlyUsed(it->second);
    return pos->second;
  } else {
    DebugError("Fail to find file" + FormatPath(filePath));
    return shared_ptr<File>();
  }
}

// --------------------------------------------------------------------------
CacheListIterator Cache::Begin() {
  lock_guard<recursive_mutex> locker(m_mutex);
  return m_cache.begin();
}

// --------------------------------------------------------------------------
CacheListIterator Cache::End() {
  lock_guard<recursive_mutex> locker(m_mutex);
  return m_cache.end();
}

// --------------------------------------------------------------------------
boost::shared_ptr<File> Cache::MakeFile(const string &fileId) {
  lock_guard<recursive_mutex> locker(m_mutex);
  CacheListIterator pos = UnguardedNewEmptyFile(fileId);
  if (pos != m_cache.end()) {
    return pos->second;
  } else {
    return shared_ptr<File>();
  }
}

// --------------------------------------------------------------------------
bool Cache::Write(const string &fileId, off_t offset, size_t len,
                  const char *buffer, const shared_ptr<DirectoryTree> &dirTree,
                  bool open) {
  lock_guard<recursive_mutex> locker(m_mutex);
  if (len == 0) {
    CacheMapIterator it = m_map.find(fileId);
    if (it != m_map.end()) {
      UnguardedMakeFileMostRecentlyUsed(it->second);
    } else {
      UnguardedNewEmptyFile(fileId);
    }
    return true;  // do nothing
  }

  bool validInput =
      !fileId.empty() && offset >= 0 && len >= 0 && buffer != NULL;
  assert(validInput);
  if (!validInput) {
    DebugError("Try to write cache with invalid input " +
               ToStringLine(fileId, offset, len, buffer));
    return false;
  }

  DebugInfo("Write cache [offset:len=" + to_string(offset) + ":" +
            to_string(len) + "] " + FormatPath(fileId));
  pair<bool, shared_ptr<File> > res = PrepareWrite(fileId, len);
  bool success = res.first;
  if (success) {
    shared_ptr<File> &file = res.second;
    assert(file);
    tuple<bool, size_t, size_t> res =
        file->Write(offset, len, buffer, open);
    success = boost::get<0>(res);
    if (success) {
      m_size += boost::get<1>(res);  // added size in cache
    }
  }

  // update file size in meta data
  if (success && dirTree) {
    shared_ptr<Node> node = dirTree->Find(fileId);
    if (node) {
      uint64_t oldFileSize = node->GetFileSize();
      if (offset + len > oldFileSize) {
        node->SetFileSize(offset + len);
      }
      node->SetFileOpen(open);
    }
  }

  return success;
}

// --------------------------------------------------------------------------
bool Cache::Write(const string &fileId, off_t offset, size_t len,
                  const shared_ptr<iostream> &stream,
                  const shared_ptr<DirectoryTree> &dirTree, bool open) {
  lock_guard<recursive_mutex> locker(m_mutex);
  if (len == 0) {
    CacheMapIterator it = m_map.find(fileId);
    if (it != m_map.end()) {
      UnguardedMakeFileMostRecentlyUsed(it->second);
    } else {
      UnguardedNewEmptyFile(fileId);
    }
    return true;  // do nothing
  }

  bool isValidInput = !fileId.empty() && offset >= 0 && stream;
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Invalid input [file:offset=" + fileId + ":" +
               to_string(offset) + "]");
    return false;
  }

  size_t streamsize = GetStreamSize(stream);
  assert(len <= streamsize);
  if (!(len <= streamsize)) {
    DebugError(
        "Invalid input, stream buffer size is less than input 'len' parameter. "
        "[file:len=" +
        fileId + ":" + to_string(len) + "]");
    return false;
  }

  DebugInfo("Write cache [offset:len=" + to_string(offset) + ":" +
            to_string(len) + "] " + FormatPath(fileId));
  pair<bool, shared_ptr<File> > res = PrepareWrite(fileId, len);
  bool success = res.first;
  if (success) {
    shared_ptr<File> &file = res.second;
    assert(file);
    tuple<bool, size_t, size_t> res =
        file->Write(offset, len, stream, open);
    success = boost::get<0>(res);
    if (success) {
      m_size += boost::get<1>(res);  // added size in cache
    }
  }

  // update file size in meta data
  if (success && dirTree) {
    shared_ptr<Node> node = dirTree->Find(fileId);
    if (node) {
      uint64_t oldFileSize = node->GetFileSize();
      if (offset + len > oldFileSize) {
        node->SetFileSize(offset + len);
      }
      node->SetFileOpen(open);
    }
  }

  return success;
}

// --------------------------------------------------------------------------
pair<bool, shared_ptr<File> > Cache::PrepareWrite(const string &fileId,
                                                  size_t len) {
  bool availableFreeSpace = true;
  if (!HasFreeSpace(len)) {
    availableFreeSpace = Free(len, fileId);

    if (!availableFreeSpace) {
      string diskfolder =
          QS::Configure::Options::Instance().GetDiskCacheDirectory();
      if (!CreateDirectoryIfNotExists(diskfolder)) {
        Error("Unable to mkdir for folder " + FormatPath(diskfolder));
        return make_pair(false, shared_ptr<File>());
      }
      if (!IsSafeDiskSpace(diskfolder, len)) {
        if (!FreeDiskCacheFiles(diskfolder, len, fileId)) {
          Error("No available free space (" + to_string(len) +
                "bytes) for folder " + FormatPath(diskfolder));
          return make_pair(false, shared_ptr<File>());
        }
      }  // check safe disk space
    }
  }

  CacheListIterator pos = m_cache.begin();
  CacheMapIterator it = m_map.find(fileId);
  if (it != m_map.end()) {
    pos = UnguardedMakeFileMostRecentlyUsed(it->second);
  } else {
    pos = UnguardedNewEmptyFile(fileId);
    assert(pos != m_cache.end());
  }

  shared_ptr<File> &file = pos->second;
  file->SetUseDiskFile(!availableFreeSpace);

  return make_pair(true, file);
}

// --------------------------------------------------------------------------
bool Cache::Free(size_t size, const string &fileUnfreeable) {
  lock_guard<recursive_mutex> locker(m_mutex);
  if (size > GetCapacity()) {
    DebugInfo("Try to free cache of " + to_string(size) +
              " bytes which surpass the maximum cache size(" +
              to_string(GetCapacity()) + " bytes). Do nothing");
    return false;
  }
  if (HasFreeSpace(size)) {
    // DebugInfo("Try to free cache of " + to_string(size) +
    //           " bytes while free space is still available. Go on");
    return true;
  }

  assert(!m_cache.empty());
  size_t freedSpace = 0;
  size_t freedDiskSpace = 0;

  CacheList::reverse_iterator it = m_cache.rbegin();
  // Discards the least recently used File first, which is put at back.
  while (it != m_cache.rend() && !HasFreeSpace(size)) {
    // Notice do NOT store a reference of the File supposed to be removed.
    string fileId = it->first;
    if (fileId != fileUnfreeable && it->second && !it->second->IsOpen()) {
      size_t fileCacheSz = it->second->GetCachedSize();
      freedSpace += fileCacheSz;
      freedDiskSpace += it->second->GetSize() - fileCacheSz;
      m_size -= fileCacheSz;
      it->second->Clear();
      m_cache.erase((++it).base());
      m_map.erase(fileId);
    } else {
      if (!it->second) {
        DebugInfo("file in cache is null " + FormatPath(fileId));
        m_cache.erase((++it).base());
        m_map.erase(fileId);
      } else {
        ++it;
      }
    }
  }

  if (freedSpace > 0) {
    Info("Has freed cache of " + to_string(freedSpace) + " bytes for file " +
         FormatPath(fileUnfreeable));
  }
  if (freedDiskSpace > 0) {
    Info("Has freed disk file of " + to_string(freedDiskSpace) +
         " bytes for file " + FormatPath(fileUnfreeable));
  }
  return HasFreeSpace(size);
}

// --------------------------------------------------------------------------
bool Cache::FreeDiskCacheFiles(const string &diskfolder, size_t size,
                               const string &fileUnfreeable) {
  assert(diskfolder ==
         QS::Configure::Options::Instance().GetDiskCacheDirectory());
  lock_guard<recursive_mutex> locker(m_mutex);
  // diskfolder should be cache disk dir
  if (IsSafeDiskSpace(diskfolder, size)) {
    return true;
  }

  assert(!m_cache.empty());
  size_t freedSpace = 0;
  size_t freedDiskSpace = 0;

  CacheList::reverse_iterator it = m_cache.rbegin();
  // Discards the least recently used File first, which is put at back.
  while (it != m_cache.rend() && !IsSafeDiskSpace(diskfolder, size)) {
    // Notice do NOT store a reference of the File supposed to be removed.
    string fileId = it->first;
    if (fileId != fileUnfreeable && it->second && !it->second->IsOpen()) {
      size_t fileCacheSz = it->second->GetCachedSize();
      freedSpace += fileCacheSz;
      freedDiskSpace += it->second->GetSize() - fileCacheSz;
      m_size -= fileCacheSz;
      it->second->Clear();
      m_cache.erase((++it).base());
      m_map.erase(fileId);
    } else {
      if (!it->second) {
        DebugInfo("file in cache is null " + FormatPath(fileId));
        m_cache.erase((++it).base());
        m_map.erase(fileId);
      } else {
        ++it;
      }
    }
  }

  if (freedSpace > 0) {
    Info("Has freed cache of " + to_string(freedSpace) + " bytes for file " +
         FormatPath(fileUnfreeable));
  }
  if (freedDiskSpace > 0) {
    Info("Has freed disk file of " + to_string(freedDiskSpace) +
         " bytes for file" + FormatPath(fileUnfreeable));
  }
  return IsSafeDiskSpace(diskfolder, size);
}

// --------------------------------------------------------------------------
CacheListIterator Cache::Erase(const string &fileId) {
  lock_guard<recursive_mutex> locker(m_mutex);
  CacheMapIterator it = m_map.find(fileId);
  if (it != m_map.end()) {
    DebugInfo("Erase cache " + FormatPath(fileId));
    return UnguardedErase(it);
  } else {
    DebugInfo("File not exist, no remove " + FormatPath(fileId));
    return m_cache.end();
  }
}

// --------------------------------------------------------------------------
void Cache::Rename(const string &oldFileId, const string &newFileId) {
  if (oldFileId == newFileId) {
    DebugInfo("File exists, no rename " + FormatPath(oldFileId));
    return;
  }
  lock_guard<recursive_mutex> locker(m_mutex);
  CacheMapIterator iter = m_map.find(newFileId);
  if (iter != m_map.end()) {
    DebugWarning("File exist, Just remove it from cache " +
                 FormatPath(newFileId));
    UnguardedErase(iter);
  }

  CacheMapIterator it = m_map.find(oldFileId);
  if (it != m_map.end()) {
    it->second->first = newFileId;
    CacheListIterator pos = UnguardedMakeFileMostRecentlyUsed(it->second);
    pos->second->Rename(newFileId);

    pair<CacheMapIterator, bool> res = m_map.emplace(newFileId, pos);
    if (!res.second) {
      DebugWarning("Fail to rename " + FormatPath(oldFileId, newFileId));
    }
    m_map.erase(it);
    DebugInfo("Renamed file in cache" + FormatPath(oldFileId, newFileId));
  } else {
    DebugInfo("File not exists, no rename " + FormatPath(oldFileId, newFileId));
  }
}

// --------------------------------------------------------------------------
void Cache::SetFileOpen(const std::string &fileId, bool open,
                        const shared_ptr<DirectoryTree> &dirTree) {
  lock_guard<recursive_mutex> locker(m_mutex);
  CacheMapIterator it = m_map.find(fileId);
  if (it != m_map.end()) {
    shared_ptr<File> &file = it->second->second;
    file->SetOpen(open);
  } else {
    DebugInfo("File not exists, no set open" + FormatPath(fileId));
  }

  if (dirTree) {
    shared_ptr<Node> node = dirTree->Find(fileId);
    if (node) {
      node->SetFileOpen(open);
    }
  }
}

// --------------------------------------------------------------------------
void Cache::Resize(const string &fileId, size_t newFileSize,
                   const shared_ptr<DirectoryTree> &dirTree) {
  lock_guard<recursive_mutex> locker(m_mutex);
  CacheMapIterator it = m_map.find(fileId);
  CacheListIterator pos;
  if (it != m_map.end()) {
    pos = it->second;
  } else {
    // File not cached yet, maybe because its content's empty
    pos = UnguardedNewEmptyFile(fileId);
  }

  shared_ptr<File> &file = pos->second;
  size_t oldFileSize = file->GetSize();
  size_t oldFileCacheSize = file->GetCachedSize();
  if (newFileSize == oldFileSize) {
    return;  // do nothing
  } else if (newFileSize > oldFileSize) {
    // fill the hole
    size_t holeSize = newFileSize - oldFileSize;
    vector<char> hole(holeSize);  // value initialization with '\0'
    DebugInfo("Fill hole [offset:len=" + to_string(oldFileSize) + ":" +
              to_string(holeSize) + "] " + FormatPath(fileId));
    bool fileOpen = file->IsOpen();
    Write(fileId, oldFileSize, holeSize, &hole[0], dirTree, fileOpen);
  } else {
    file->ResizeToSmallerSize(newFileSize);
  }
  m_size += file->GetCachedSize() - oldFileCacheSize;

  if (file->GetSize() == newFileSize) {
    if (dirTree) {
      shared_ptr<Node> node = dirTree->Find(fileId);
      if (node) {
        node->SetFileSize(newFileSize);
      }
    }
  } else {
    DebugWarning("Try to resize file from size " + to_string(oldFileSize) +
                 " to " + to_string(newFileSize) + ". But now file size is " +
                 to_string(file->GetSize()) + FormatPath(fileId));
  }
}

// --------------------------------------------------------------------------
CacheListIterator Cache::UnguardedNewEmptyFile(const string &fileId) {
  m_cache.push_front(make_pair(fileId, make_shared<File>(fileId)));
  if (m_cache.begin()->first == fileId) {  // insert to cache sucessfully
    pair<CacheMapIterator, bool> res = m_map.emplace(fileId, m_cache.begin());
    if (res.second) {
      return m_cache.begin();
    } else {
      DebugError("Fail to create empty file in cache " + FormatPath(fileId));
      return m_cache.end();
    }
  } else {
    DebugError("Fail to create empty file in cache " + FormatPath(fileId));
    return m_cache.end();
  }
}

// --------------------------------------------------------------------------
CacheListIterator Cache::UnguardedErase(
    FileIdToCacheListIteratorMap::iterator pos) {
  CacheListIterator cachePos = pos->second;
  shared_ptr<File> &file = cachePos->second;
  m_size -= file->GetCachedSize();
  file->Clear();
  CacheListIterator next = m_cache.erase(cachePos);
  m_map.erase(pos);
  return next;
}

// --------------------------------------------------------------------------
CacheListIterator Cache::UnguardedMakeFileMostRecentlyUsed(
    CacheListIterator pos) {
  m_cache.splice(m_cache.begin(), m_cache, pos);
  // no iterators or references become invalidated, so no need to update m_map.
  return m_cache.begin();
}

}  // namespace Data
}  // namespace QS
