/*
    board.cpp   
   
    This file is part of Samuel. 

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

// Gui Checkers by Jonathan Kreuzer
// copyright 2005

// =================================================
// BOARD Representation. I changed to this representation after seeing code by Martin Fierz
// everything else in this file I believe was made without looking at other checkers source of any kind.
/*              37  38  39  40
              32  33  34  35
                28  29  30  31
              23  24  25  26
                19  20  21  22
              14  15  16  17
                10  11  12  13
               5   6   7   8         */
//
// I also use my own bitboard representation: See http://www.3dkingdoms.com/checkers/bitboards.htm
//
// =================================================

#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <cstdio>

#define TRUE 1
#define FALSE 0

//#define KING_VAL 33

extern int g_bDatabases;
extern unsigned int HASHTABLESIZE;
extern int g_nMoves;
extern char *g_sNameVer;
extern char cpath[10];

#define PD 0
const int KV = KING_VAL;
const int KBoard[64] = { PD, PD, PD, PD, PD,
                         KV-3,  KV+0,  KV+0,  KV+0, PD,
                            KV+0,  KV+1,  KV+1,  KV+0,
                         KV+1,  KV+3,  KV+3,  KV+2, PD,
                            KV+3,  KV+5,  KV+5,  KV+1,
                         KV+1,  KV+5,  KV+5,  KV+3, PD,
                            KV+2,  KV+3,  KV+3,  KV+1,
                         KV+0,  KV+1,  KV+1,  KV+0, PD,
                            KV+0,  KV+0,  KV+0, KV-3};

struct CBoard
{
    void StartPosition( int bResetRep );
    void Clear();
    void SetFlags();
    int EvaluateBoard (int ahead, int alpha, int beta);
    int InDatabase() {
        if (!g_bDatabases || nWhite > 2 || nBlack > 2 ) return 0;
        return 1;
    }
    void ToFen(char *sFEN);
    int FromFen(char *sFEN);
    int FromPDN(char *sPDN);
    int ToPDN(char *sPDN);

    int MakeMovePDN( int src, int dst);
    int DoMove( SMove &Move, int nType);
    void DoSingleJump( int src, int dst, int nPiece);
    void CheckKing( int src, int dst, int nPiece);
    void UpdateSqTable( int nPiece, int src, int dst );

    // Data
    char Sqs[48];
    SCheckerBoard C;
    short nPSq, eval;
    char nWhite, nBlack, SideToMove, extra;
    u_int64_t HashKey;
};

CBoard g_Boardlist[MAX_SEARCHDEPTH + 1];
CBoard g_CBoard;
CBoard g_StartBoard;

extern int QueryDatabase( CBoard &Board );

//void DrawBoard (const CBoard &board);
//int QueryDatabase( CBoard &Board);

int BoardLoc[66] =
    {   0, 37, 0, 38, 0, 39, 0, 40, 32, 0, 33, 0, 34, 0, 35, 0, 0, 28, 0, 29, 0 ,30, 0, 31, 23, 0, 24, 0, 25, 0, 26, 0,
        0, 19, 0, 20, 0, 21, 0, 22, 14, 0, 15, 0, 16, 0, 17, 0, 0, 10, 0, 11, 0, 12, 0, 13, 5, 0, 6, 0, 7, 0, 8, 0};

// =================================================
//
//              TRANSPOSITION TABLE
//
// =================================================
unsigned char g_ucAge = 0;

struct TEntry
    {
    // FUNCTIONS
    public:
    void inline Read( unsigned long CheckSum, short alpha, short beta, int &bestmove, int &value, int depth, int ahead)
        {
        if (m_checksum == CheckSum ) //To be almost totally sure these are really the same position.
            {
            int tempVal;
            // Get the Value if the search was deep enough, and bounds usable
            if (m_depth >= depth)
               {
               if (abs(m_eval) > 1800) // This is a game ending value, must adjust it since it depends on the variable ahead
                    {
                    if (m_eval > 0) tempVal = m_eval - ahead + m_ahead;
                    if (m_eval < 0) tempVal = m_eval + ahead - m_ahead;
                    }
               else tempVal = m_eval;
               switch ( m_failtype )
                    {
                    case 0: value = tempVal;  // Exact value
                        break;
                    case 1: if (tempVal <= alpha) value = tempVal; // Alpha Bound (check to make sure it's usuable)
                        break;
                    case 2: if (tempVal >= beta) value = tempVal; //  Beta Bound (check to make sure it's usuable)
                        break;
                    }
               }
            // Otherwise take the best move from Transposition Table
            bestmove = m_bestmove;
            }
        }

    void inline Write( unsigned long CheckSum, short alpha, short beta, int &bestmove, int &value, int depth, int ahead)
        {
        if (m_age == g_ucAge && m_depth > depth && m_depth > 14) return; // Don't write over deeper entries from same search
        m_checksum = CheckSum;
        m_eval = value;
        m_ahead = ahead;
        m_depth = depth;
        m_bestmove = bestmove;
        m_age = g_ucAge;
        if (value <= alpha) m_failtype = 1;
        else if (value >= beta)  m_failtype = 2;
        else m_failtype = 0;
        }

    static void Create_HashFunction();
    static u_int64_t HashBoard( const CBoard &Board );

    // DATA
    private:
        unsigned long m_checksum;
        short m_eval;
        short m_bestmove;
        char m_depth;
        char m_failtype, m_ahead;
        unsigned char m_age;
    };

// Declare and allocate the transposition table
TEntry *TTable = (TEntry *) malloc( sizeof (TEntry) * HASHTABLESIZE + 2);

// ------------------
//  Hash Board
//
//  HashBoard is called to get the hash value of the start board.
// ------------------
u_int64_t HashFunction[48][12], HashSTM;

void TEntry::Create_HashFunction( )
    {
    for (int i = 0; i <48; i++)
        for (int x = 0; x < 9; x++)
            {HashFunction [i][x]  = rand() + (rand()*256) + (rand()*65536);
             HashFunction [i][x] <<= 32;
             HashFunction [i][x]  += rand() + (rand()*256) + (rand()*65536);
            }
    HashSTM = HashFunction[0][0];
    }

u_int64_t TEntry::HashBoard( const CBoard &Board )
    {
    u_int64_t CheckSum = 0;

    for (int index = 5; index <= 40; index++)
        {
        int nPiece =  Board.Sqs [ index ];
        if (nPiece != EMPTY) CheckSum ^= HashFunction [ index ][ nPiece ];
        }
    if (Board.SideToMove == BLACK)
        {
        CheckSum ^= HashSTM;
        }
    return CheckSum;
    }

// Check for a repeated board
// (only checks every other move, because side to move must be the same)
int64_t RepNum[ MAX_GAMEMOVES ];

// with Hashvalue passed
int inline Repetition( const int64_t HashKey, int nStart, int ahead)
//int inline Repetition( long long HashKey, int nStart, int ahead)
    {
    int i;
    if (nStart > 0) i = nStart; else i = 0;

    if ((i&1) != (ahead&1)) i++;

    ahead-=2;
    for ( ; i < ahead; i+=2)
         if (RepNum[i] == HashKey)
            return TRUE;

    return FALSE;
    }

void inline AddRepBoard( const int64_t HashKey, int ahead )
    {
    RepNum[ ahead ] = HashKey;
    }

// =================================================
//
//              BOARD FUNCTIONS
//
// =================================================
void CBoard::Clear( )
    {
    int i;
    for (i = 0; i< 48; i++) Sqs [i] = EMPTY;
    for (i = 0; i < 5; i++) Sqs [i] = INVALID;
    for (i = 41; i <  48; i++) Sqs [i] = INVALID;
    Sqs[9] = INVALID; Sqs[18] = INVALID; Sqs[27] = INVALID; Sqs[36] = INVALID;
    SetFlags ();
    }

void CBoard::StartPosition( int bResetRep )
   {
    Clear ();
    int i;
    for (i = 5; i <= 8; i++)  Sqs [i] = BPIECE;
    for (i = 10; i <= 17; i++) Sqs [i] = BPIECE;
    for (i = 28; i <= 35; i++) Sqs [i] = WPIECE;
    for (i = 37; i <= 40; i++) Sqs [i] = WPIECE;

    SideToMove = BLACK;
    if (bResetRep) g_nMoves = 0;

    SetFlags ();
    g_StartBoard = *this;
   }

void CBoard::SetFlags( )
    {
    nWhite = 0; nBlack = 0;
    nPSq = 0;
    for (int i = 5; i < 41; i++)
        {
        if ( (Sqs[i]&WHITE) && Sqs[i]<INVALID) {nWhite++;
            if (Sqs[i] == WKING) {nPSq += KBoard[i];}
            }
        if ( (Sqs[i]&BLACK) && Sqs[i]<INVALID) {nBlack++;
            if (Sqs[i] == BKING) {nPSq -= KBoard[i];}
            }
        }
    HashKey = TEntry::HashBoard( *this );
    C.ConvertFromSqs( Sqs );
    }

// ------------------
//  Evaluate Board
//
//  I don't know much about checkers, this is the best I could do for an evaluation function
// ------------------
const int LAZY_EVAL_MARGIN = 64;

int CBoard::EvaluateBoard (int ahead, int alpha, int beta)
    {
    // Game is over?
    if (C.WP == 0) return -2001 + ahead;
    if (C.BP == 0) return 2001 - ahead;
    //
    int eval, square;

    // Number of pieces present. Scaled higher for less material
    if ((nWhite+nBlack) > 12 ) eval = (nWhite-nBlack) * 100;
        else eval = (nWhite-nBlack) * (160 - (nWhite + nBlack) * 5 );
    eval+=nPSq;

    // Probe the W/L/D bitbase
    if ( InDatabase() )
    {        
        int Result = QueryDatabase( *this );
        if (Result == 2) eval+=400;
        if (Result == 1) eval-=400;
        if (Result == 0) return 0;
    }
    else {if (nWhite == 1 && nBlack >= 3 && nBlack < 8) eval = eval + (eval>>1); // surely winning advantage
          if (nBlack == 1 && nWhite >= 3 && nWhite < 8) eval = eval + (eval>>1);
         }

    // Too far from the alpha beta window? Forget about the rest of the eval, it probably won't get value back within the window
    if (eval+LAZY_EVAL_MARGIN < alpha) return eval;
    if (eval-LAZY_EVAL_MARGIN > beta) return eval;

    // Keeping checkers off edges
    eval -= 2 * (BitCount(C.WP & ~C.K & MASK_EDGES) - BitCount(C.BP & ~C.K & MASK_EDGES) );
    // Mobility
    eval += 2 * ( C.GetWhiteMoves( ) - C.GetBlackMoves( ) );

    // The losing side can make it tough to win the game by moving a King back and forth on the double corner squares.
    if (eval > 18)
        {if ((C.BP & C.K & MASK_2CORNER1)) {eval-=8; if ((MASK_2CORNER1 & ~C.BP &~C.WP)) eval-=10;}
         if ((C.BP & C.K & MASK_2CORNER2)) {eval-=8; if ((MASK_2CORNER2 & ~C.BP &~C.WP)) eval-=10;}
        }
    if (eval < -18)
        {if ((C.WP & C.K & MASK_2CORNER1)) {eval+=8; if ((MASK_2CORNER1 & ~C.BP &~C.WP)) eval+=10;}
         if ((C.WP & C.K & MASK_2CORNER2)) {eval+=8; if ((MASK_2CORNER2 & ~C.BP &~C.WP)) eval+=10;}
        }

    int nWK = BitCount( C.WP & C.K );
    int nBK = BitCount( C.BP & C.K );
    int WPP = C.WP & ~C.K;
    int BPP = C.BP & ~C.K;
    // Advancing checkers in endgame
    if ( (nWK*2) >= nWhite || (nBK*2) >= nBlack || (nBK+nWK)>=4) {
        int Mul;
        if ( nWK == nBK && (nWhite+nBlack) < (nWK+nBK+2) ) Mul = 8; else Mul = 2;
        eval -=  Mul * ( BitCount( (WPP & MASK_TOP) ) - BitCount( (WPP & MASK_BOTTOM) )
                      -BitCount( (BPP & MASK_BOTTOM) ) + BitCount( (BPP & MASK_TOP) ) );
        }

    static int BackRowGuardW[8] = { 0, 4, 5, 13, 4, 20, 18, 25 };
    static int BackRowGuardB[8] = { 0, 4, 5, 20, 4, 18, 13, 25 };
    int nBackB = 0, nBackW = 0;
    if ( (nWK*2) < nWhite )
        {
         nBackB = (( BPP ) >> 1) & 7;
         eval -= BackRowGuardB[ nBackB ];
        }
    if ( (nBK*2) < nBlack )
        {
         nBackW = (( WPP ) >> 28) & 7;
         eval += BackRowGuardW[ nBackW ];
        }

    // Number of Active Kings
    int nAWK = nWK, nABK = nBK;
    // Kings trapped on back row
    if ( C.WP & C.K & MASK_TRAPPED_W ) {
        if (Sqs[5]  == WKING && Sqs[6] == BPIECE) {eval-=22; nAWK--; if (Sqs[14] != EMPTY) eval+=7;}
        if (Sqs[10] == WKING && Sqs[14] !=EMPTY && Sqs[6]  == BPIECE) {eval-=10; nAWK--;}
        if (Sqs[6]  == WKING && Sqs[15] == EMPTY && Sqs[5] == BPIECE && Sqs[7] == BPIECE) {eval-=14; nAWK--;}
        if (Sqs[7]  == WKING && Sqs[16] == EMPTY && Sqs[6] == BPIECE && Sqs[8] == BPIECE) {eval-=14; nAWK--;}
        if (Sqs[8]  == WKING && Sqs[13] != EMPTY && Sqs[7] == BPIECE && Sqs[17] == EMPTY) {eval-=16; nAWK--;}
    }
    if ( C.BP & C.K & MASK_TRAPPED_B ) {
        if (Sqs[37] == BKING && Sqs[32] != EMPTY && Sqs[38] == WPIECE && Sqs[28] == EMPTY) {eval+=16; nABK--;}
        if (Sqs[38] == BKING && Sqs[29] == EMPTY && Sqs[37] == WPIECE && Sqs[39] == WPIECE) {eval+=14; nABK--;}
        if (Sqs[39] == BKING && Sqs[30] == EMPTY && Sqs[40] == WPIECE && Sqs[38] == WPIECE) {eval+=14; nABK--;}
        if (Sqs[40] == BKING && Sqs[39] == WPIECE) {eval+=22; nABK--; if (Sqs[31] != EMPTY) eval-=7; }
        if (Sqs[35] == BKING && Sqs[39] == WPIECE && Sqs[31] != EMPTY) {eval+=10; nABK--;}
    }

    // Reward checkers that will king on the next move
    int KSQ = 0;
    if ( (BPP & MASK_7TH) ) {
        for (square = 32; square <= 35; square++)
            {
            if (Sqs[ square ] == BPIECE)
                {if (Sqs[ square + 4] == EMPTY) {KSQ = square+4; eval-=(KBoard [square+4] - 6); break;}
                 if (Sqs[ square + 5] == EMPTY) {KSQ = square+5; eval-=(KBoard [square+5] - 6); break;}
                }
            }
        } else square = 36;
    // Opponent has no Active Kings and backrow is still protected, so give a bonus
    if (nAWK > 0 && (square == 36 || KSQ == 40) && nABK == 0)
        {eval+=20;
         if (nAWK > 1) eval+=10;
         if (BackRowGuardW[ nBackW ] > 10) eval += 15;
         if (BackRowGuardW[ nBackW ] > 20) eval += 20;
        }

    if ( (WPP & MASK_2ND) ) {
        for (square = 13; square >= 10; square--)
            {
            if (Sqs[ square ] == WPIECE)
                {if (Sqs[ square - 4] == EMPTY )  {KSQ = square-4; eval+=(KBoard[square-4] - 6); break;}
                 if (Sqs[ square - 5] == EMPTY )  {KSQ = square-5; eval+=(KBoard[square-5] - 6); break;}
                }
            }
        } else square = 9;
    if (nABK > 0 && (square == 9 || KSQ == 5) && nAWK == 0)
        {eval-=20;
         if (nABK > 1) eval-=10;
         if (BackRowGuardB[ nBackB ] > 10) eval -= 15;
         if (BackRowGuardB[ nBackB ] > 20) eval -= 20;
        }

    if (nWhite == nBlack ) // equal number of pieces, but one side has all kinged versus all but one kinged (or all kings)
        {// score should be about equal, unless it's in a database, then don't reduce
        if (eval > 0 && eval < 200  && nWK >= nBK && nBK >= nWhite-1) eval = (eval>>1) + (eval>>3);
        if (eval < 0 && eval > -200 && nBK >= nWK && nWK >= nBlack-1) eval = (eval>>1) + (eval>>3);
        }

    return eval;
    }

//
// MOVE EXECUTION
//
// ---------------------
//  Helper Functions for DoMove
// ---------------------
void inline CBoard::DoSingleJump( const int src, const int dst, const int nPiece )
{    
    int jumpedSq = (dst + src) >> 1;
    int jumpedPiece = Sqs[ jumpedSq ];
    if ( jumpedPiece == WPIECE ) {nWhite--; }
        else if ( jumpedPiece == BPIECE) {nBlack--; }
        else if ( jumpedPiece == BKING)  {nBlack--; nPSq += KBoard[jumpedSq]; }
        else if ( jumpedPiece == WKING)  {nWhite--; nPSq -= KBoard[jumpedSq]; }
    
    HashKey ^= HashFunction[ src ][ nPiece ] ^ HashFunction[ dst ][ nPiece ] ^ HashFunction[ jumpedSq ][ jumpedPiece ];
    
    // Update the board array
    Sqs[ dst ] = nPiece;
    Sqs[ jumpedSq ] = EMPTY;
    Sqs[ src ] = EMPTY;

    // Update the bitboards
    UINT BitMove = (S[ BoardLocTo32[src] ] | S[ BoardLocTo32[dst] ]);
    if (SideToMove == BLACK) {
        C.WP ^= BitMove;
        C.BP &= ~S[BoardLocTo32[jumpedSq]];
        C.K  &= ~S[BoardLocTo32[jumpedSq]];
    } else {
        C.BP ^= BitMove;
        C.WP &= ~S[BoardLocTo32[jumpedSq]];
        C.K  &= ~S[BoardLocTo32[jumpedSq]];
    }
    if (nPiece & KING) C.K ^= BitMove;    
}

// This function will test if a checker needs to be upgraded to a king, and upgrade if necessary
void inline CBoard::CheckKing (const int src, const int dst, int const nPiece)
{
    if (dst <= 8)
        {Sqs[ dst ] = WKING;
         nPSq += KBoard[dst];
         HashKey  ^= HashFunction[ dst ][ nPiece ] ^ HashFunction[ dst ][ WKING ];
         C.K |= S[BoardLocTo32[dst]];
        }
    if (dst >= 37)
        {Sqs[ dst ] = BKING;
         nPSq -= KBoard[dst];
         HashKey ^= HashFunction[ dst ][ nPiece ] ^ HashFunction[ dst ][ BKING ];
         C.K |= S[BoardLocTo32[dst]];
        }
}

void inline CBoard::UpdateSqTable( const int nPiece, const int src, const int dst )
{
    switch (nPiece) {
        case BKING: nPSq -= KBoard[ dst ] - KBoard[ src ]; return;
        case WKING: nPSq += KBoard[ dst ] - KBoard[ src ]; return;
    }
}

// ---------------------
//  Execute a Move
// ---------------------
int CBoard::DoMove (SMove &Move, int nType)
    {
    unsigned int src = (Move.SrcDst&63);
    unsigned int dst = (Move.SrcDst>>6)&63;
    unsigned int bJump = (Move.SrcDst>>12);
    unsigned int nPiece = Sqs[src];    

    SideToMove ^=3;
    HashKey ^= HashSTM; // Flip hash stm

    if (bJump == 0)
        {        
        // Update the board array
        Sqs[ dst ] = Sqs[ src ];
        Sqs[ src ] = EMPTY;

        // Update the bitboards
        UINT BitMove = (S[ BoardLocTo32[src] ] | S[ BoardLocTo32[dst] ]);
        if (SideToMove == BLACK) C.WP ^= BitMove; else C.BP ^= BitMove;
        if (nPiece & KING) C.K ^= BitMove;
        
        // Update hash values
        HashKey ^= HashFunction[ src ][ nPiece ] ^ HashFunction[ dst ][ nPiece ];

        UpdateSqTable( nPiece, src, dst );
       
        if (nPiece < KING)  {
             CheckKing( src, dst, nPiece );
             return 1;}
        return 0;
        }

    DoSingleJump ( src, dst, nPiece);
    if (nPiece < KING) CheckKing( src, dst, nPiece );
    
    // Double Jump?
    if (Move.cPath[0] == 0)
        {UpdateSqTable ( nPiece, src, dst );
         return 1;
        }
    if (nType == HUMAN) return DOUBLEJUMP;    

    for (int i = 0; i < 8; i++)
        {        
        int nDst = Move.cPath[i];          
        if (nDst == 0) break;       

        if (nType == MAKEMOVE) // pause a bit on displaying double jumps
            {
            if (i == 0)
                {
                cpath[0] = src;
                cpath[1] = dst;
                }            
            cpath[i + 2] = nDst; // set variable in enginemodule for python gui     
            //DrawBoard ( *this );             
             // Sleep (300);             
             //sleep (1);
            }
        DoSingleJump( dst, nDst, nPiece);
        dst = nDst;
        }    
    if (nPiece < KING) CheckKing( src, dst, nPiece );
    else UpdateSqTable ( nPiece, src, dst );    
    return 1;
    }

// Flip square horizontally because the internal board is flipped.
long FlipX( int x )
{
    int y = x&3;
    x ^= y;
    x += 3-y;
    return x;
}
// ------------------
// Position Copy & Paste Functions
// ------------------

void CBoard::ToFen ( char *sFEN )
{
    char buffer[80];
    int i;
    if (SideToMove == WHITE) strcpy( sFEN, "W:");
    else strcpy( sFEN, "B:");

    strcat( sFEN, "W");
    for (i = 0; i < 32; i++) {
        if ( Sqs[ BoardLoc32[i] ] == WKING) strcat(sFEN, "K");
        if ( Sqs[ BoardLoc32[i] ]&WPIECE)  {
            //strcat( sFEN, _ltoa (FlipX(i)+1, buffer, 10) );
            //strcat( sFEN, ltoa (FlipX(i)+1, buffer, 10) );            
            //char test[10];
            //sprintf(test,"%l",fsize);
            sprintf(buffer,"%ld",FlipX(i)+1);            
            strcat(sFEN, buffer);
            strcat( sFEN, ","); }
        }
    if (strlen(sFEN) > 3) sFEN [ strlen(sFEN)-1 ] = NULL;
    strcat( sFEN, ":B");
    for (i = 0; i < 32; i++) {
        if ( Sqs[ BoardLoc32[i] ] == BKING) strcat(sFEN, "K");
        if ( Sqs[ BoardLoc32[i] ]&BPIECE)  {
            //strcat( sFEN, _ltoa (FlipX(i)+1, buffer, 10) );
            sprintf(buffer,"%ld",FlipX(i)+1);
            strcat(sFEN, buffer);
            strcat( sFEN, ","); }
        }
    sFEN[ strlen(sFEN)-1 ] = '.';
}

int CBoard::FromFen ( char *sFEN )
{
    //DisplayText( sFEN );
    if ((sFEN[0] != 'W' && sFEN[0]!='B') || sFEN[1]!=':')
    {        
        return 0;
    }
    Clear ();
    if (sFEN[0] == 'W') SideToMove = WHITE;
    if (sFEN[0] == 'B') SideToMove = BLACK;

    int nColor=0, i=0;
    while (sFEN[i] != 0 && sFEN[i]!= '.' && sFEN[i-1]!= '.')
        {
        int nKing = 0;
        if (sFEN[i] == 'W') nColor = WPIECE;
        if (sFEN[i] == 'B') nColor = BPIECE;
        if (sFEN[i] == 'K') {nKing = 4; i++; }
        if (sFEN[i] >= '0' && sFEN[i] <= '9')
            {
            int sq = sFEN[i]-'0';
            i++;
            if (sFEN[i] >= '0' && sFEN[i] <= '9') sq = sq*10 + sFEN[i]-'0';
            Sqs[ BoardLoc32[ FlipX(sq-1) ] ] = nColor | nKing;
            }
        i++;
        }

    SetFlags ( );
    g_StartBoard = *this;
    return 1;
}

// For PDN support
//
int GetFinalDst( SMove &Move )
    {
    int sq = ((Move.SrcDst>>6)&63);
    if ( (Move.SrcDst>>12) )
        for (int i = 0; i < 8; i++)
            {
            if (Move.cPath[i] == 0) break;
            sq = Move.cPath[i];
            }
    return sq;
    }

//
//
int CBoard::MakeMovePDN( int src, int dst)
    {
    CMoveList Moves;
    if (SideToMove == BLACK) Moves.FindMovesBlack( Sqs, C );
    if (SideToMove == WHITE) Moves.FindMovesWhite( Sqs, C );
    for (int x = 1; x <= Moves.nMoves; x++)
        {
        int nMove = Moves.Moves[x].SrcDst;
        if ( (nMove&63) == src )
            if ( ((nMove>>6)&63) == dst || GetFinalDst( Moves.Moves[x] ) == dst )
            {
            DoMove ( Moves.Moves[x], SEARCHED );
            g_GameMoves [ g_nMoves++ ] = Moves.Moves[x];
            g_GameMoves [ g_nMoves ].SrcDst = 0;
            return 1;
            }
        }
    return 0;
    }

//
//
int CBoard::FromPDN ( char *sPDN )
{
    int i = 0, nEnd = strlen (sPDN);
    int src = 0, dst = 0;
    char sFEN[512];

    StartPosition ( 1 );

    while ( i < nEnd )
        {
        if ( !memcmp (&sPDN[i], "1-0",3) || !memcmp (&sPDN[i], "0-1",3) || !memcmp (&sPDN[i], "1/2-1/2",7) || sPDN[i]=='*')
            break;
        if ( !memcmp (&sPDN[i], "[FEN \"",6))
            {
            i+=6;
            int x = 0;
            while (sPDN[i] !='"' && i < nEnd) sFEN[x++] = sPDN[i++];
            sFEN[x++] = 0;
            FromFen( sFEN );
            }
        if (sPDN[i] == '[') while (sPDN[i] != ']' && i < nEnd) i++;
        if (sPDN[i] == '{') while (sPDN[i] != '}' && i < nEnd) i++;
        if (sPDN[i] >= '0' && sPDN[i] <= '9' && sPDN[i+1] == '.') i++;
        if (sPDN[i] >= '0' && sPDN[i] <= '9')
            {
            int sq = sPDN[i]-'0';
            i++;
            if (sPDN[i] >= '0' && sPDN[i] <= '9') sq = sq*10 + sPDN[i]-'0';
            src = BoardLoc32[ FlipX(sq-1) ];
            }
        if ((sPDN[i] == '-' || sPDN[i] == 'x')
            && sPDN[i+1] >= '0' && sPDN[i+1] <= '9')
            {
            i++;
            int sq = sPDN[i]-'0';
            i++;
            if (sPDN[i] >= '0' && sPDN[i] <= '9') sq = sq*10 + sPDN[i]-'0';
            dst = BoardLoc32[ FlipX(sq-1) ];

            MakeMovePDN ( src, dst);
            }
        i++;
        }
    //DisplayText (sPDN);
    return 1;
}

int CBoard::ToPDN ( char *sPDN )
{
    sPDN[0] = 0;
    int i = 0, j = 0;
    char cType;
    j+=sprintf ( sPDN+j, "[Event \"%s game\"]\015\012", g_sNameVer);

    char sFEN[512];
    g_StartBoard.ToFen ( sFEN );
    if ( strcmp ( sFEN, "B:W24,23,22,21,28,27,26,25,32,31,30,29:B4,3,2,1,8,7,6,5,12,11,10,9.") )
        {
        j+=sprintf ( sPDN+j, "[SetUp \"1\"]\015\012" );
        j+=sprintf ( sPDN+j, "[FEN \"%s\"]\015\012", sFEN );
        }

    while (g_GameMoves[i].SrcDst !=0)
        {
        if ((i%2)==0) j+=sprintf ( sPDN+j, "%d. ", (i/2)+1 );
        unsigned int src = (g_GameMoves[i].SrcDst&63);
        unsigned int dst = GetFinalDst( g_GameMoves[i] );
        if ( (g_GameMoves[i].SrcDst>>12) ) cType = 'x'; else cType = '-';
        j+=sprintf ( sPDN+j, "%ld%c%ld ", FlipX(BoardLocTo32[ src ])+1, cType, FlipX(BoardLocTo32[ dst ])+1 );
        i++;
        if ((i%12)==0) j+=sprintf ( sPDN+j, "\015\012" );
        }
    sprintf ( sPDN+j, "*" );
    return 1;
}
