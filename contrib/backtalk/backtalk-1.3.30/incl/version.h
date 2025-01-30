/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Backtalk Version Number
 *
 * Backtalk stores it's version number in all compiled files and will
 * automatically recompile them if they don't match.  Because of this,
 * we need to be a bit more systematic than average about version numbers.
 *
 * Each Backtalk release will be identified by the first three version numbers:
 *     <VERSION_A>.<VERSION_B>.<VERSION_C>
 * The first two numbers will change with major upgrades.  The third digit is
 * incremented for bug fixes and minor revisions.  There is a fourth digit,
 * <VERSION_D> which is not normally displayed.  It is for *non-release*
 * development increments.  Basically, any time during development that I
 * change the interpreter enough so that I want to recompile all test scripts,
 * I increment VERSION_D.
 */

#define VERSION_A  1
#define VERSION_B  3
#define VERSION_C  30
#define VERSION_D  0

#define VERSION_NOTE ""
/*#define VERSION_NOTE " (beta)"*/

#define COPYRIGHT_STRING "Copyright 1996-2006, Jan Wolter and Steve Weiss"
