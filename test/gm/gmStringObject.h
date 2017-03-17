/*
    _____               __  ___          __            ____        _      __
   / ___/__ ___ _  ___ /  |/  /__  ___  / /_____ __ __/ __/_______(_)__  / /_
  / (_ / _ `/  ' \/ -_) /|_/ / _ \/ _ \/  '_/ -_) // /\ \/ __/ __/ / _ \/ __/
  \___/\_,_/_/_/_/\__/_/  /_/\___/_//_/_/\_\\__/\_, /___/\__/_/ /_/ .__/\__/
                                               /___/             /_/
                                             
  See Copyright Notice in gmMachine.h

*/

#ifndef _GMSTRINGOBJECT_H_
#define _GMSTRINGOBJECT_H_

#include "gmConfig.h"
#include "gmVariable.h"
#include "gmHash.h"

class gmMachine;

/// \class gmStringObject
/// \brief
class gmStringObject : public gmObject, public gmHashNode<const char *, gmStringObject>
{
public:

  const char * GetKey() const { GM_ASSERT(m_string); return m_string; }

  virtual int GetType() const { return GM_STRING; }
  virtual void Destruct(gmMachine * a_machine);

  operator const char *() const { GM_ASSERT(m_string); return m_string; }
  const char * GetString() const { GM_ASSERT(m_string); return m_string; }
  int GetLength() const { return m_length; }

  bool operator==(const char * a_string) const { GM_ASSERT(m_string && a_string); return strcmp(a_string, m_string) == 0; }
  bool operator!=(const char * a_string) const { GM_ASSERT(m_string && a_string); return strcmp(a_string, m_string) != 0; }

protected:

  /// \brief Non-public constructor.  Create via gmMachine.
  gmStringObject(const char * a_string, int a_length) { m_string = a_string; m_length = a_length; }
  friend class gmMachine;

private:

  const char * m_string;
  int m_length;
};

#endif // _GMSTRINGOBJECT_H_
