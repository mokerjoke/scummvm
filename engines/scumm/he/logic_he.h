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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#if !defined(SCUMM_HE_LOGIC_HE_H) && defined(ENABLE_HE)
#define SCUMM_HE_LOGIC_HE_H

namespace Scumm {

class ScummEngine_v90he;

class LogicHE {
public:
	ScummEngine_v90he *_vm;

	LogicHE(ScummEngine_v90he *vm);
	virtual ~LogicHE();

	void writeScummVar(int var, int32 value);
	int getFromArray(int arg0, int idx2, int idx1);
	void putInArray(int arg0, int idx2, int idx1, int val);

	virtual void beforeBootScript() {}
	virtual void initOnce() {}
	virtual int startOfFrame() { return 1; }
	void endOfFrame() {}
	void processKeyStroke(int keyPressed) {}

	virtual int versionID();
	virtual int32 dispatch(int op, int numArgs, int32 *args);
};

class LogicHErace : public LogicHE {
private:
	float *_userData;
	double *_userDataD;
public:
	LogicHErace(ScummEngine_v90he *vm);
	~LogicHErace();

	int versionID();
	int32 dispatch(int op, int numArgs, int32 *args);

private:
	int32 op_1003(int32 *args);
	int32 op_1004(int32 *args);
	int32 op_1100(int32 *args);
	int32 op_1101(int32 *args);
	int32 op_1102(int32 *args);
	int32 op_1103(int32 *args);
	int32 op_1110();
	int32 op_1120(int32 *args);
	int32 op_1130(int32 *args);
	int32 op_1140(int32 *args);

	void op_sub1(float arg);
	void op_sub2(float arg);
	void op_sub3(float arg);
};

class LogicHEfunshop : public LogicHE {
public:
	LogicHEfunshop(ScummEngine_v90he *vm) : LogicHE(vm) {}

	int versionID();
	int32 dispatch(int op, int numArgs, int32 *args);

private:
	void op_1004(int32 *args);
	void op_1005(int32 *args);
	int checkShape(int32 data0, int32 data1, int32 data4, int32 data5, int32 data2, int32 data3, int32 data6, int32 data7, int32 *x, int32 *y);
};

class LogicHEfootball : public LogicHE {
public:
	LogicHEfootball(ScummEngine_v90he *vm) : LogicHE(vm) {}

	int versionID();
	int32 dispatch(int op, int numArgs, int32 *args);

private:
	int op_1004(int32 *args);
	int op_1006(int32 *args);
	int op_1007(int32 *args);
	int op_1010(int32 *args);
	int op_1022(int32 *args);
	int op_1023(int32 *args);
	int op_1024(int32 *args);
};

class LogicHEsoccer : public LogicHE {
private:
	double *_userDataD;

public:
	LogicHEsoccer(ScummEngine_v90he *vm);
	~LogicHEsoccer();

	int versionID();
	int32 dispatch(int op, int numArgs, int32 *args);

	void beforeBootScript();
	void initOnce();
	int startOfFrame();

private:
	int op_1001(int32 *args);
	int op_1002(int32 *args);
	int op_1003(int32 *args);
	int op_1004(int32 *args);
	int op_1005(float a1, float a2, float a3, float a4, float a5, float a6, float *a7, float *a8, float *a9, float *a10);
	int op_1006(int32 a1, int32 a2, int32 a3, int32 a4);
	int op_1007(int32 *args);
	int op_1008(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14, int a15, int a16, int a17, int a18, int a19);
	int op_1011(int32 a1, int32 a2, int32 a3, int32 a4, int32 a5, int32 a6);
	int op_1012(int32 *args);
	int op_1013(int32 a1, int32 a2, int32 a3);
	int op_1014(int32 a1, int32 a2, int32 a3, int32 a4, int32 a5, int32 a6, int32 a7, int32 a8, int32 a9, int32 a10, int32 a11, int32 a12, int32 a13, int32 a14);
	int op_1016(int32 *args);
	int op_1017(int32 *args);
	int op_1019(int32 *args);
	int op_1021(int32 a1, int32 a2, int32 a3, int32 a4, int32 a5, int32 a6, int32 a7);

	// op_1007 allocates some arrays
	// they're then filled by op_1019
	byte _byteArray1[4096], _byteArray2[585];

	// op_1011 has a subfunction
	void op_1011_sub(int32 a1, int32 a2, int32 a3, int32 a4);

	// op_1013 creates some array, purpose unknown
	bool _array1013Allocated;
	uint32 *_array1013;
	int op_sub5(int a1, int a2, int a3);

	// op_1014 has several subops
	// ...and several sub-subops
	int op_1014_sub0(float a1, float a2, float a3, float a4, float a5, float a6);
	int op_1014_sub0_0(int a1, int a2, uint32 *a3, int a4);
	void op_1014_sub0_0_0(byte a1);
	int op_1014_sub1(int a1, float a2, float a3, float a4, float a5, float *a6, float &a7, float &a8, float &a9, int a10, int a11, float *a12, float *a13, float *a14, float *a15);
	void op_1014_sub1_0(int a1, float &a2, float &a3, float &a4, float &a5, float &a6, float &a7, float &a8, float &a9, float &a10, float &a11, float &a12, float &a13, int a14[24]);
	void op_1014_sub1_1(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float &a13, float &a14, float &a15);
	double op_1014_sub1_2(float a1, float a2, float a3, float a4, float a5, float a6);
	void op_1014_sub2(float *a1, int a2, int a3, int a4);
	int op_1014_sub3(float *a1, int a2, int a3, int a4, int a5, int a6, int a7, float a8, int a9, int a10, int a11, int *a12);

	// op_1014 sets an array optionally based upon op_1014_sub3
	// This array is then used by op_1008
	int _array1014[10];
	Common::List<byte> _list1014;

	// op_1021 can (optionally) set two variables for use in op_1008
	uint32 _var1021[2];
};

class LogicHEbaseball2001 : public LogicHE {
public:
	LogicHEbaseball2001(ScummEngine_v90he *vm) : LogicHE(vm) {}

	int versionID();
	int32 dispatch(int op, int numArgs, int32 *args);
};

class LogicHEbasketball : public LogicHE {
public:
	LogicHEbasketball(ScummEngine_v90he *vm) : LogicHE(vm) {}

	int versionID();
	int32 dispatch(int op, int numArgs, int32 *args);
};

class LogicHEmoonbase : public LogicHE {
public:
	LogicHEmoonbase(ScummEngine_v90he *vm) : LogicHE(vm) {}

	int versionID();
};

} // End of namespace Scumm

#endif
