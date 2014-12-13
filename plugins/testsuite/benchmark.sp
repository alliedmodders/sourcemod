#include <sourcemod>
#include <profiler>

public Plugin:myinfo = 
{
	name = "Benchmarks",
	author = "AlliedModders LLC",
	description = "Basic benchmarks",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

#define MATH_INT_LOOPS		2000
#define MATH_FLOAT_LOOPS	2000
#define STRING_OP_LOOPS		2000
#define STRING_FMT_LOOPS	2000
#define STRING_ML_LOOPS		2000
#define STRING_RPLC_LOOPS	2000

new Float:g_dict_time
new Handle:g_Prof = null

public OnPluginStart()
{
	RegServerCmd("bench", Benchmark);
	g_Prof = CreateProfiler();
	StartProfiling(g_Prof);
	LoadTranslations("fakedict-sourcemod.cfg");
	StopProfiling(g_Prof);
	g_dict_time = GetProfilerTime(g_Prof);
}

public Action:Benchmark(args)
{
	PrintToServer("dictionary time: %f seconds", g_dict_time);
	StringBench();
	MathBench();
	return Plugin_Handled;
}

MathBench()
{
	StartProfiling(g_Prof);
	new iter = MATH_INT_LOOPS;
	new a, b, c;
	while(iter--)
	{
		a = iter * 7;
		b = 5 + iter;
		c = 6 / (iter + 3);
		a = 6 * (iter);
		b = a * 185;
		a = b / 25;
		c = b - a + 3;
		b = b*b;
		a = (a + c) / (b - c);
		b = 6;
		c = 1;
		b = a * 128 - c;
		c = b * (a + 16) * b;
		if (!a)
		{
			a = 5;
		}
		a = c + (28/a) - c;
	}
	StopProfiling(g_Prof);
	PrintToServer("int benchmark: %f seconds", GetProfilerTime(g_Prof));

	StartProfiling(g_Prof);	
	new Float:fa, Float:fb, Float:fc
	new int1
	iter = MATH_FLOAT_LOOPS;
	while (iter--)
	{
		fa = iter * 0.7;
		fb = 5.1 + iter;
		fc = 6.1 / (float(iter) + 2.5);
		fa = 6.1 * (iter);
		fb = fa * 185.26;
		fa = fb / 25.56;
		fc = fb - a + float(3);
		fb = fb*fb;
		fa = (fa + fc) / (fb - fc);
		fb = 6.2;
		fc = float(1);
		int1 = RoundToNearest(fa);
		fb = fa * float(128) - int1;
		fc = fb * (a + 16.85) * float(RoundToCeil(fb));
		if (fa == 0.0)
		{
			fa = 5.0;
		}
		fa = fc + (float(28)/fa) - RoundToFloor(fc);
	}
	StopProfiling(g_Prof);
	PrintToServer("float benchmark: %f seconds", GetProfilerTime(g_Prof));
}

#define KEY1	"LVWANBAGVXSXUGB"
#define KEY2	"IDYCVNWEOWNND"
#define KEY3	"UZWTRNHY"
#define KEY4	"EPRHAFCIUOIG"
#define KEY5	"RMZCVWIEY"
#define KEY6	"ZHPU"

StringBench()
{
	new i = STRING_FMT_LOOPS;
	new String:buffer[255];
	
	StartProfiling(g_Prof);
	new end
	while (i--)
	{
		end = 0;
		Format(buffer, sizeof(buffer), "%d", i);
		Format(buffer, sizeof(buffer), "%d %s %d %f %d %-3.4s %s", i, "gaben", 30, 10.0, 20, "hello", "What a gaben");
		end = Format(buffer, sizeof(buffer), "Well, that's just %-17.18s!", "what.  this isn't a valid string! wait it is");
		end += Format(buffer[end], sizeof(buffer)-end, "There are %d in this %d", i, end);
		end += Format(buffer[end], sizeof(buffer)-end, "There are %d in this %d", i, end);
	}
	StopProfiling(g_Prof);
	PrintToServer("format() benchmark: %f seconds", GetProfilerTime(g_Prof));
	
	StartProfiling(g_Prof);
	i = STRING_ML_LOOPS;
	new String:fmtbuf[2048];	/* don't change to decl, amxmodx doesn't use it */
	while (i--)
	{
		Format(fmtbuf, 2047, "%T %T %d %s %f %T", KEY1, LANG_SERVER, KEY2, LANG_SERVER, 50, "what the", 50.0, KEY3, LANG_SERVER);
		Format(fmtbuf, 2047, "%s %T %s %T %T", "gaben", KEY4, LANG_SERVER, "what TIME is it", KEY5, LANG_SERVER, KEY6, LANG_SERVER);
	}
	StopProfiling(g_Prof);
	PrintToServer("ml benchmark: %f seconds", GetProfilerTime(g_Prof));
	
	StartProfiling(g_Prof);
	i = STRING_OP_LOOPS;
	while (i--)
	{
		StringToInt(fmtbuf)
	}
	StopProfiling(g_Prof);
	PrintToServer("str benchmark: %f seconds", GetProfilerTime(g_Prof));
	
	StartProfiling(g_Prof);
	i = STRING_RPLC_LOOPS;
	while (i--)
	{
		strcopy(fmtbuf, 2047, "This is a test string for you.");
		ReplaceString(fmtbuf, sizeof(fmtbuf), " ", "ASDF")
		ReplaceString(fmtbuf, sizeof(fmtbuf), "SDF", "")
		ReplaceString(fmtbuf, sizeof(fmtbuf), "string", "gnirts")
	}
	StopProfiling(g_Prof);
	PrintToServer("replace benchmark: %f seconds", GetProfilerTime(g_Prof));
}
