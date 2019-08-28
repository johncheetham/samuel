/*
    obook.cpp   
   
    This file is part of Samuel

    This file comes from the guicheckers project.
    See http://www.3dkingdoms.com/checkers.htm

    It has been amended for use in Samuel.

    Samuel is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Samuel is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Samuel.  If not, see <http://www.gnu.org/licenses/>.
   
 */

#include <cstdio>
#include <ctime>

#include "ai.hh"

#define BOOK_EXTRA_SIZE 200000
#define NOVAL -30000

void DisplayText (const char *sText);

// =======================================
// OPENING BOOK
// for Gui Checkers
// =======================================
class COpeningBook
{
 // structures needed
 struct SBookEntry
 {
  SBookEntry ()
    {
    pNext = NULL;
    wValue = NOVAL;
    }
  ~SBookEntry ()
    {
    }
  void Save (FILE *pFile, unsigned int wPosKey )
    {
    wPosKey &= 65535;
    if (fwrite (&ulCheck, 4, 1, pFile) != 1)
    {
        cout << "error writing data1" << endl;
    }

    if (fwrite (&wKey   , 2, 1, pFile) != 1)
    {
        cout << "error writing data2" << endl;
    }

    if (fwrite (&wValue , 2, 1, pFile) != 1)
    {
        cout << "error writing data3" << endl;
    }

    if (fwrite (&wPosKey , 2, 1, pFile)  != 1)
    {
        cout << "error writing data4" << endl;
    }

    }
  unsigned int Load (FILE *pFile )
  {
    unsigned int wPosKey = 0;
    if (fread (&ulCheck, 4, 1, pFile) != 1)
    {
        //cout << "error reading data1" << endl;
    }

    if (fread (&wKey   , 2, 1, pFile) != 1)
    {
        //cout << "error reading data2" << endl;
    }

    if (fread (&wValue , 2, 1, pFile) != 1)
    {
        //cout << "error reading data3" << endl;
    }

    if (fread (&wPosKey , 2, 1, pFile) != 1)
    {
        //cout << "error reading data4" << endl;
    }
    
    return wPosKey;
  }

  unsigned long ulCheck;
  SBookEntry* pNext;
  unsigned short wKey;
  short wValue;
 };

 public:
 // ctor&dtor
    COpeningBook ( )
    {
    m_pHash = new SBookEntry[ 131072 ];
    m_pExtra = new SBookEntry[ BOOK_EXTRA_SIZE ];
    m_nListSize = 0;
    m_nPositions = 0;
    }

    ~COpeningBook ( )
    {
    delete [] m_pHash;
    delete [] m_pExtra;
    }

 // Functions
 void LearnGame ( int nMoves, int Adjust );
 int  FindMoves ( CBoard &Board, int Moves[], short wValues[] );
 int  GetMove ( CBoard &Board, int &nBestMove );
 void RemovePosition ( CBoard &Board, int bQuiet );
 int  GetValue ( CBoard &Board );
 void AddPosition ( CBoard &Board, short wValue, int bQuiet );
 void AddPosition ( unsigned long ulKey,  unsigned long ulCheck, short wValue, int bQuiet );
 int  Load (char *sFileName );
 void Save (char *sFileName );

 // Data
 SBookEntry *m_pHash;
 SBookEntry *m_pExtra;
 int m_nListSize;
 int m_nPositions;
};

// ============================================
//
//   OPENING BOOK Functions. 
//
// ============================================
void COpeningBook::AddPosition ( CBoard &Board, short wValue, int bQuiet )
{
 unsigned long ulKey   = (unsigned long)Board.HashKey;
 unsigned long ulCheck = (unsigned long)(Board.HashKey>>32);
 AddPosition (ulKey, ulCheck, wValue, bQuiet );
}

//
//
int COpeningBook::GetValue ( CBoard &Board )
{
 unsigned long ulKey   = (unsigned long)Board.HashKey;
 unsigned long ulCheck = (unsigned long)(Board.HashKey>>32);

 int nEntry = ulKey&131071;
 SBookEntry *pEntry;

 pEntry = &m_pHash[ nEntry ];
 while (pEntry != NULL && pEntry->wValue != NOVAL)
    {
    if (pEntry->ulCheck == ulCheck  && pEntry->wKey == (ulKey>>16) )
        {
        return pEntry->wValue;
        }
    pEntry = pEntry->pNext;
    }

return NOVAL;
}

//
//
//
void COpeningBook::AddPosition( unsigned long ulKey,  unsigned long ulCheck, short wValue, int bQuiet )
{
    //char msg[ 255 ];
    int nEntry = ulKey&131071;

    SBookEntry *pEntry = &m_pHash[ nEntry ];     

    while (pEntry->wValue != NOVAL)
    {
        if (pEntry->ulCheck == ulCheck && pEntry->wKey == (ulKey>>16) )
        {            
            if (wValue == 0)
            {
                if (pEntry->wValue > 0) wValue--;
                if (pEntry->wValue < 0) wValue++;
            }
            if (!bQuiet) 
            {
                    sprintf( msg, _("Position Exists. %d\nValue was %d now %d"), nEntry, pEntry->wValue, pEntry->wValue + wValue);
                    DisplayText ( msg );
            }
            pEntry->wValue += wValue;
            return;
        }
        if (pEntry->pNext == NULL)
        {
            if (m_nListSize >= BOOK_EXTRA_SIZE)
            {
                DisplayText (_("Book Full!"));
                return;
            }
            pEntry->pNext = &m_pExtra[ m_nListSize ];
            m_nListSize++;
        }
        pEntry = pEntry->pNext;
    }

    // New Position    

    m_nPositions++;
    pEntry->ulCheck = ulCheck;
    pEntry->wKey = (unsigned short) (ulKey>>16);    
    pEntry->wValue = wValue;
    if (!bQuiet)
    {
        sprintf ( msg, _("Position Added. %d\nValue %d"), nEntry, wValue);
        //DisplayText ( msg );
    }
}

// ----------------------------------
//  Remove a position from the book.
//  (if this position is in the list, it stays in memory until the book is saved and loaded.)
// ----------------------------------
void COpeningBook::RemovePosition( CBoard &Board, int bQuiet )
{
    //char msg[ 255 ];

    // ulKey changed from unsigned long to unsigned int 
    // it needs to be 4 bytes and unsigned long is 8 bytes on 64 bit Linux
    //unsigned long ulKey   = (unsigned long)Board.HashKey; 
    unsigned int ulKey   = (unsigned int)Board.HashKey;    

    unsigned long ulCheck = (unsigned long)(Board.HashKey>>32);
    int nEntry = ulKey&131071;           // 01FFFF
    SBookEntry *pEntry = &m_pHash[ nEntry ];
    SBookEntry *pPrev = NULL; 

    while (pEntry != NULL && pEntry->wValue != NOVAL)
    {        
        if (pEntry->ulCheck == ulCheck && pEntry->wKey == (ulKey>>16) )         
        {
            m_nPositions--;
            if (!bQuiet)
            {
                sprintf ( msg, _("Position Removed. %d\nValue was %d"), nEntry, m_pHash [ nEntry ].wValue);
                //DisplayText ( msg );
            }
            if (pPrev != NULL)
                pPrev->pNext = pEntry->pNext;
            else 
            {
                if (pEntry->pNext != NULL) *pEntry = *pEntry->pNext;
                    else pEntry->wValue = NOVAL;
            }
            return;
        }
        pPrev = pEntry;
        pEntry = pEntry->pNext;
    }

    if (!bQuiet)
    {
        sprintf ( msg, _("Position does not exist. %d\n"),nEntry);
        DisplayText ( msg );
    }
}

// --------------------
//
// FILE I/O For Opening Book
//
// --------------------
// LOAD

int COpeningBook::Load( char *sFileName )
{
    FILE *fp = fopen( sFileName, "rb");
    SBookEntry TempEntry;
    int i = 0;

    if ( fp == NULL ) return false;

    unsigned long wKey = TempEntry.Load( fp );
    while (!feof( fp ) )
    {
        i++;
        wKey += (TempEntry.wKey << 16);
        AddPosition( wKey, TempEntry.ulCheck, TempEntry.wValue, TRUE );
        wKey = TempEntry.Load( fp );
    }

    fclose ( fp );

    //char msg[ 255 ];
    sprintf( msg, _("%d Positions Loaded"), i);
    DisplayText( msg );
    cout << "Opening Book database loaded from " << sFileName << endl;
    return true;
}

//
// SAVE
//
// the book generation code muddles this up a bit, nTimes is 
// the number of times this position must have occured to be saved.
//
void COpeningBook::Save ( char *sFileName )
{
    FILE *fp;
    fp = fopen (sFileName, "wb");
    if (fp == NULL) return;

    SBookEntry *pEntry;
    int Num = 0;

    for (int i = 0; i < 131072; i++)
    {
        pEntry = &m_pHash [ i ];
        if (pEntry->wValue != NOVAL)
        {
        if (pEntry->wValue < 100 ) {
                pEntry->Save ( fp, i );
                Num++;}

        while (pEntry->pNext != NULL)
            {
            pEntry = pEntry->pNext;
            pEntry->Save ( fp, i );
            Num++;
            }
        }
    }
    fclose ( fp );

    //char msg[ 255 ];
    sprintf ( msg, _("%d Positions Saved"), Num);
    //DisplayText ( msg );
}

// --------------------
// Find a book move by doing all legal moves, then checking to see if the position is in the opening book.
// --------------------
int COpeningBook::FindMoves( CBoard &Board, int OutMoves[], short wValues[] )
{
int i, val, nFound;
CMoveList Moves;
CBoard TempBoard;

if (Board.SideToMove == BLACK) Moves.FindMovesBlack( Board.Sqs, Board.C );
if (Board.SideToMove == WHITE) Moves.FindMovesWhite( Board.Sqs, Board.C );

nFound = 0;

for (i = 1; i <= Moves.nMoves; i++)
    {
    TempBoard = Board;
    TempBoard.DoMove( Moves.Moves[i], SEARCHED  );
    val = GetValue ( TempBoard );    
    // Add move if it leads to a position in the book 
    if ( val != NOVAL) 
        {
        OutMoves [ nFound ] = i;
        wValues  [ nFound ] = val;
        nFound++;
        }
    }

return nFound;
}

// --------------------
// Try to do a move in the opening book
// --------------------
int COpeningBook::GetMove ( CBoard &Board, int &nBestMove )
{
    int nVal = NOVAL, nMove = -1;
    int nMax, i, nGood;
    int Moves [ 60 ];
    int GoodMoves [ 60 ];
    short wValues [60], wGoodVal [60];

    int nFound = FindMoves ( Board, Moves, wValues );
    
    srand( (unsigned)time( 0 ) ); // Randomize

    nGood = 0;
    nMax = 0;
    for (i = 0; i < nFound; i++)
    {
        if ( (Board.SideToMove == BLACK && wValues[i] <= 0) || (Board.SideToMove == WHITE && wValues[i] >= 0) )
        {
            if (abs(wValues[i]) > nMax) nMax = abs(wValues[i]);
        }
    }

    for (i = 0; i < nFound; i++) 
    {
        if ( (Board.SideToMove == BLACK && wValues[i] <= 0) || (Board.SideToMove == WHITE && wValues[i] >= 0) )
        {
            if (wValues[i] == 0 && nMax > 0) continue;
            wGoodVal [nGood] = wValues [i];
            GoodMoves[nGood] = Moves[i];
            nGood++;
        }
    }

    if (nGood == 0)
    {        
        return NOVAL;
    }

    nMove = rand()%nGood;
    nVal = wGoodVal[ nMove ];

    if (abs(nVal) < abs(nMax))
    {
        nMove = rand()%nGood;
        nVal = wGoodVal[ nMove ];
    }
    if (abs(nVal) < abs(nMax)-1)
    {
        nMove = rand()%nGood;
        nVal = wGoodVal[ nMove ];
    }

    if (nMove!=-1) nBestMove = GoodMoves[ nMove ]; 
    else nBestMove = NONE;

    return nVal;
}

//
// Book Learning
//
void COpeningBook::LearnGame ( int nMoves, int nAdjust )
{
for (int i = 0; i <= (nMoves-1)*2; i++)
    {
    unsigned long ulKey   = (unsigned long) RepNum[ i ];
    unsigned long ulCheck = (unsigned long)(RepNum[ i ]>>32);
    AddPosition ( ulKey, ulCheck, nAdjust, true );
    }
}
