#pragma once

class CCritSec 
{ 
public: 
	CCritSec(void)
	{ 
		InitializeCriticalSection(&m_cs); 
	} 

	~CCritSec(void) 
	{ 
		DeleteCriticalSection(&m_cs); 
	} 

	void Lock(void) 
	{ 
		EnterCriticalSection(&m_cs); 
	} 

	void Unlock(void) 
	{ 
		LeaveCriticalSection(&m_cs); 
	} 

private: 
	CRITICAL_SECTION m_cs; 
}; 

class CAutoLock 
{ 
public: 
	CAutoLock(CCritSec* pLock) : m_pLock(pLock) 
	{ 
		m_pLock->Lock(); 
	} 

	~CAutoLock(void) 
	{ 
		m_pLock->Unlock(); 
	} 

private: 
	CCritSec* m_pLock; 
}; 