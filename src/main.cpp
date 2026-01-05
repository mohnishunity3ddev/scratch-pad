// #include <stdio.h>

// #define LINEAR_ALLOCATOR_IMPLEMENTATION
// #define LINEAR_ALLOCATOR_UNIT_TESTS
// #include <memory/linear_alloc.h>
// #define STACK_ALLOCATOR_IMPLEMENTATION
// #define STACK_ALLOCATOR_UNIT_TEST
// #include <memory/stack_alloc_forward.h>
// #define POOL_ALLOCATOR_IMPLEMENTATION
// #define POOL_ALLOCATOR_UNIT_TEST
// #include <memory/pool_alloc.h>
// #include "common.h"
// #include <stddef.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <assert.h>
// #include <string.h>
// #include <memory/memory.h>

// #include "clock.h"
// #define STRING32_IMPLEMENTATION
// #include <containers/string_utils.h>

// #ifndef HASTHABLE_IMPLEMENTATION
// #define HASHTABLE_IMPLEMENTATION
// #endif
// #include <containers/htable.h>

// #define FREELIST_ALLOCATOR_UNIT_TESTS
// #define FREELIST_ALLOCATOR_IMPLEMENTATION
// #include <memory/freelist_alloc.h>
// #define FREELIST2_ALLOCATOR_UNIT_TESTS
// #define FREELIST2_ALLOCATOR_IMPLEMENTATION
// #include <memory/freelist2_alloc.h>

// #include "memory/handle.h"

// #define RBT_IMPLEMENTATION
// #include <containers/rb_tree.h>

// #include "clock.h"

// #include <utility>
// #include <math/vec.h>
// #include <math/mat.h>
// #include <memory/bitmapped_alloc.h>

// #include "containers/stack.hpp"
// #include <cassert>
// #include <cstdint>
// #include <memory/sebi_pool.h>
// #include <containers/OffsetAllocator/offsetAllocator.hpp>

#include <memory/tlsf.h>

int main(int argc, char **argv)
{
    Tlsf::Allocator allocator;

    /*
    // uint32_t binIndex0 = allocator.allocate(0); assert(binIndex0 == 0);
    uint32_t binIndex1 = allocator.allocate(1); assert(binIndex1 == 1);
    uint32_t binIndex2 = allocator.allocate(2); assert(binIndex2 == 2);
    uint32_t binIndex3 = allocator.allocate(3); assert(binIndex3 == 3);
    uint32_t binIndex4 = allocator.allocate(4); assert(binIndex4 == 4);
    uint32_t binIndex5 = allocator.allocate(5); assert(binIndex5 == 5);
    uint32_t binIndex6 = allocator.allocate(6); assert(binIndex6 == 6);
    uint32_t binIndex7 = allocator.allocate(7); assert(binIndex7 == 7);
    uint32_t binIndex8 = allocator.allocate(8); assert(binIndex8 == 8);
    uint32_t binIndex9 = allocator.allocate(9); assert(binIndex9 == 9);
    uint32_t binIndex10 = allocator.allocate(10); assert(binIndex10 == 10);
    uint32_t binIndex11 = allocator.allocate(11); assert(binIndex11 == 11);
    uint32_t binIndex12 = allocator.allocate(12); assert(binIndex12 == 12);
    uint32_t binIndex13 = allocator.allocate(13); assert(binIndex13 == 13);
    uint32_t binIndex14 = allocator.allocate(14); assert(binIndex14 == 14);
    uint32_t binIndex15 = allocator.allocate(15); assert(binIndex15 == 15);
    uint32_t binIndex16 = allocator.allocate(16); assert(binIndex16 == 16);
    uint32_t binIndex17 = allocator.allocate(18); assert(binIndex17 == 17);
    uint32_t binIndex18 = allocator.allocate(20); assert(binIndex18 == 18);
    uint32_t binIndex19 = allocator.allocate(22); assert(binIndex19 == 19);
    uint32_t binIndex20 = allocator.allocate(24); assert(binIndex20 == 20);
    uint32_t binIndex21 = allocator.allocate(26); assert(binIndex21 == 21);
    uint32_t binIndex22 = allocator.allocate(28); assert(binIndex22 == 22);
    uint32_t binIndex23 = allocator.allocate(30); assert(binIndex23 == 23);
    uint32_t binIndex24 = allocator.allocate(32); assert(binIndex24 == 24);
    uint32_t binIndex25 = allocator.allocate(36); assert(binIndex25 == 25);
    uint32_t binIndex26 = allocator.allocate(40); assert(binIndex26 == 26);
    uint32_t binIndex27 = allocator.allocate(44); assert(binIndex27 == 27);
    uint32_t binIndex28 = allocator.allocate(48); assert(binIndex28 == 28);
    uint32_t binIndex29 = allocator.allocate(52); assert(binIndex29 == 29);
    uint32_t binIndex30 = allocator.allocate(56); assert(binIndex30 == 30);
    uint32_t binIndex31 = allocator.allocate(60); assert(binIndex31 == 31);
    uint32_t binIndex32 = allocator.allocate(64); assert(binIndex32 == 32);
    uint32_t binIndex33 = allocator.allocate(72); assert(binIndex33 == 33);
    uint32_t binIndex34 = allocator.allocate(80); assert(binIndex34 == 34);
    uint32_t binIndex35 = allocator.allocate(88); assert(binIndex35 == 35);
    uint32_t binIndex36 = allocator.allocate(96); assert(binIndex36 == 36);
    uint32_t binIndex37 = allocator.allocate(104); assert(binIndex37 == 37);
    uint32_t binIndex38 = allocator.allocate(112); assert(binIndex38 == 38);
    uint32_t binIndex39 = allocator.allocate(120); assert(binIndex39 == 39);
    uint32_t binIndex40 = allocator.allocate(128); assert(binIndex40 == 40);
    uint32_t binIndex41 = allocator.allocate(144); assert(binIndex41 == 41);
    uint32_t binIndex42 = allocator.allocate(160); assert(binIndex42 == 42);
    uint32_t binIndex43 = allocator.allocate(176); assert(binIndex43 == 43);
    uint32_t binIndex44 = allocator.allocate(192); assert(binIndex44 == 44);
    uint32_t binIndex45 = allocator.allocate(208); assert(binIndex45 == 45);
    uint32_t binIndex46 = allocator.allocate(224); assert(binIndex46 == 46);
    uint32_t binIndex47 = allocator.allocate(240); assert(binIndex47 == 47);
    uint32_t binIndex48 = allocator.allocate(256); assert(binIndex48 == 48);
    uint32_t binIndex49 = allocator.allocate(288); assert(binIndex49 == 49);
    uint32_t binIndex50 = allocator.allocate(320); assert(binIndex50 == 50);
    uint32_t binIndex51 = allocator.allocate(352); assert(binIndex51 == 51);
    uint32_t binIndex52 = allocator.allocate(384); assert(binIndex52 == 52);
    uint32_t binIndex53 = allocator.allocate(416); assert(binIndex53 == 53);
    uint32_t binIndex54 = allocator.allocate(448); assert(binIndex54 == 54);
    uint32_t binIndex55 = allocator.allocate(480); assert(binIndex55 == 55);
    uint32_t binIndex56 = allocator.allocate(512); assert(binIndex56 == 56);
    uint32_t binIndex57 = allocator.allocate(576); assert(binIndex57 == 57);
    uint32_t binIndex58 = allocator.allocate(640); assert(binIndex58 == 58);
    uint32_t binIndex59 = allocator.allocate(704); assert(binIndex59 == 59);
    uint32_t binIndex60 = allocator.allocate(768); assert(binIndex60 == 60);
    uint32_t binIndex61 = allocator.allocate(832); assert(binIndex61 == 61);
    uint32_t binIndex62 = allocator.allocate(896); assert(binIndex62 == 62);
    uint32_t binIndex63 = allocator.allocate(960); assert(binIndex63 == 63);
    uint32_t binIndex64 = allocator.allocate(1024); assert(binIndex64 == 64);
    uint32_t binIndex65 = allocator.allocate(1152); assert(binIndex65 == 65);
    uint32_t binIndex66 = allocator.allocate(1280); assert(binIndex66 == 66);
    uint32_t binIndex67 = allocator.allocate(1408); assert(binIndex67 == 67);
    uint32_t binIndex68 = allocator.allocate(1536); assert(binIndex68 == 68);
    uint32_t binIndex69 = allocator.allocate(1664); assert(binIndex69 == 69);
    uint32_t binIndex70 = allocator.allocate(1792); assert(binIndex70 == 70);
    uint32_t binIndex71 = allocator.allocate(1920); assert(binIndex71 == 71);
    uint32_t binIndex72 = allocator.allocate(2048); assert(binIndex72 == 72);
    uint32_t binIndex73 = allocator.allocate(2304); assert(binIndex73 == 73);
    uint32_t binIndex74 = allocator.allocate(2560); assert(binIndex74 == 74);
    uint32_t binIndex75 = allocator.allocate(2816); assert(binIndex75 == 75);
    uint32_t binIndex76 = allocator.allocate(3072); assert(binIndex76 == 76);
    uint32_t binIndex77 = allocator.allocate(3328); assert(binIndex77 == 77);
    uint32_t binIndex78 = allocator.allocate(3584); assert(binIndex78 == 78);
    uint32_t binIndex79 = allocator.allocate(3840); assert(binIndex79 == 79);
    uint32_t binIndex80 = allocator.allocate(4096); assert(binIndex80 == 80);
    uint32_t binIndex81 = allocator.allocate(4608); assert(binIndex81 == 81);
    uint32_t binIndex82 = allocator.allocate(5120); assert(binIndex82 == 82);
    uint32_t binIndex83 = allocator.allocate(5632); assert(binIndex83 == 83);
    uint32_t binIndex84 = allocator.allocate(6144); assert(binIndex84 == 84);
    uint32_t binIndex85 = allocator.allocate(6656); assert(binIndex85 == 85);
    uint32_t binIndex86 = allocator.allocate(7168); assert(binIndex86 == 86);
    uint32_t binIndex87 = allocator.allocate(7680); assert(binIndex87 == 87);
    uint32_t binIndex88 = allocator.allocate(8192); assert(binIndex88 == 88);
    uint32_t binIndex89 = allocator.allocate(9216); assert(binIndex89 == 89);
    uint32_t binIndex90 = allocator.allocate(10240); assert(binIndex90 == 90);
    uint32_t binIndex91 = allocator.allocate(11264); assert(binIndex91 == 91);
    uint32_t binIndex92 = allocator.allocate(12288); assert(binIndex92 == 92);
    uint32_t binIndex93 = allocator.allocate(13312); assert(binIndex93 == 93);
    uint32_t binIndex94 = allocator.allocate(14336); assert(binIndex94 == 94);
    uint32_t binIndex95 = allocator.allocate(15360); assert(binIndex95 == 95);
    uint32_t binIndex96 = allocator.allocate(16384); assert(binIndex96 == 96);
    uint32_t binIndex97 = allocator.allocate(18432); assert(binIndex97 == 97);
    uint32_t binIndex98 = allocator.allocate(20480); assert(binIndex98 == 98);
    uint32_t binIndex99 = allocator.allocate(22528); assert(binIndex99 == 99);
    uint32_t binIndex100 = allocator.allocate(24576); assert(binIndex100 == 100);
    uint32_t binIndex101 = allocator.allocate(26624); assert(binIndex101 == 101);
    uint32_t binIndex102 = allocator.allocate(28672); assert(binIndex102 == 102);
    uint32_t binIndex103 = allocator.allocate(30720); assert(binIndex103 == 103);
    uint32_t binIndex104 = allocator.allocate(32768); assert(binIndex104 == 104);
    uint32_t binIndex105 = allocator.allocate(36864); assert(binIndex105 == 105);
    uint32_t binIndex106 = allocator.allocate(40960); assert(binIndex106 == 106);
    uint32_t binIndex107 = allocator.allocate(45056); assert(binIndex107 == 107);
    uint32_t binIndex108 = allocator.allocate(49152); assert(binIndex108 == 108);
    uint32_t binIndex109 = allocator.allocate(53248); assert(binIndex109 == 109);
    uint32_t binIndex110 = allocator.allocate(57344); assert(binIndex110 == 110);
    uint32_t binIndex111 = allocator.allocate(61440); assert(binIndex111 == 111);
    uint32_t binIndex112 = allocator.allocate(65536); assert(binIndex112 == 112);
    uint32_t binIndex113 = allocator.allocate(73728); assert(binIndex113 == 113);
    uint32_t binIndex114 = allocator.allocate(81920); assert(binIndex114 == 114);
    uint32_t binIndex115 = allocator.allocate(90112); assert(binIndex115 == 115);
    uint32_t binIndex116 = allocator.allocate(98304); assert(binIndex116 == 116);
    uint32_t binIndex117 = allocator.allocate(106496); assert(binIndex117 == 117);
    uint32_t binIndex118 = allocator.allocate(114688); assert(binIndex118 == 118);
    uint32_t binIndex119 = allocator.allocate(122880); assert(binIndex119 == 119);
    uint32_t binIndex120 = allocator.allocate(131072); assert(binIndex120 == 120);
    uint32_t binIndex121 = allocator.allocate(147456); assert(binIndex121 == 121);
    uint32_t binIndex122 = allocator.allocate(163840); assert(binIndex122 == 122);
    uint32_t binIndex123 = allocator.allocate(180224); assert(binIndex123 == 123);
    uint32_t binIndex124 = allocator.allocate(196608); assert(binIndex124 == 124);
    uint32_t binIndex125 = allocator.allocate(212992); assert(binIndex125 == 125);
    uint32_t binIndex126 = allocator.allocate(229376); assert(binIndex126 == 126);
    uint32_t binIndex127 = allocator.allocate(245760); assert(binIndex127 == 127);
    uint32_t binIndex128 = allocator.allocate(262144); assert(binIndex128 == 128);
    uint32_t binIndex129 = allocator.allocate(294912); assert(binIndex129 == 129);
    uint32_t binIndex130 = allocator.allocate(327680); assert(binIndex130 == 130);
    uint32_t binIndex131 = allocator.allocate(360448); assert(binIndex131 == 131);
    uint32_t binIndex132 = allocator.allocate(393216); assert(binIndex132 == 132);
    uint32_t binIndex133 = allocator.allocate(425984); assert(binIndex133 == 133);
    uint32_t binIndex134 = allocator.allocate(458752); assert(binIndex134 == 134);
    uint32_t binIndex135 = allocator.allocate(491520); assert(binIndex135 == 135);
    uint32_t binIndex136 = allocator.allocate(524288); assert(binIndex136 == 136);
    uint32_t binIndex137 = allocator.allocate(589824); assert(binIndex137 == 137);
    uint32_t binIndex138 = allocator.allocate(655360); assert(binIndex138 == 138);
    uint32_t binIndex139 = allocator.allocate(720896); assert(binIndex139 == 139);
    uint32_t binIndex140 = allocator.allocate(786432); assert(binIndex140 == 140);
    uint32_t binIndex141 = allocator.allocate(851968); assert(binIndex141 == 141);
    uint32_t binIndex142 = allocator.allocate(917504); assert(binIndex142 == 142);
    uint32_t binIndex143 = allocator.allocate(983040); assert(binIndex143 == 143);
    uint32_t binIndex144 = allocator.allocate(1048576); assert(binIndex144 == 144);
    uint32_t binIndex145 = allocator.allocate(1179648); assert(binIndex145 == 145);
    uint32_t binIndex146 = allocator.allocate(1310720); assert(binIndex146 == 146);
    uint32_t binIndex147 = allocator.allocate(1441792); assert(binIndex147 == 147);
    uint32_t binIndex148 = allocator.allocate(1572864); assert(binIndex148 == 148);
    uint32_t binIndex149 = allocator.allocate(1703936); assert(binIndex149 == 149);
    uint32_t binIndex150 = allocator.allocate(1835008); assert(binIndex150 == 150);
    uint32_t binIndex151 = allocator.allocate(1966080); assert(binIndex151 == 151);
    uint32_t binIndex152 = allocator.allocate(2097152); assert(binIndex152 == 152);
    uint32_t binIndex153 = allocator.allocate(2359296); assert(binIndex153 == 153);
    uint32_t binIndex154 = allocator.allocate(2621440); assert(binIndex154 == 154);
    uint32_t binIndex155 = allocator.allocate(2883584); assert(binIndex155 == 155);
    uint32_t binIndex156 = allocator.allocate(3145728); assert(binIndex156 == 156);
    uint32_t binIndex157 = allocator.allocate(3407872); assert(binIndex157 == 157);
    uint32_t binIndex158 = allocator.allocate(3670016); assert(binIndex158 == 158);
    uint32_t binIndex159 = allocator.allocate(3932160); assert(binIndex159 == 159);
    uint32_t binIndex160 = allocator.allocate(4194304); assert(binIndex160 == 160);
    uint32_t binIndex161 = allocator.allocate(4718592); assert(binIndex161 == 161);
    uint32_t binIndex162 = allocator.allocate(5242880); assert(binIndex162 == 162);
    uint32_t binIndex163 = allocator.allocate(5767168); assert(binIndex163 == 163);
    uint32_t binIndex164 = allocator.allocate(6291456); assert(binIndex164 == 164);
    uint32_t binIndex165 = allocator.allocate(6815744); assert(binIndex165 == 165);
    uint32_t binIndex166 = allocator.allocate(7340032); assert(binIndex166 == 166);
    uint32_t binIndex167 = allocator.allocate(7864320); assert(binIndex167 == 167);
    uint32_t binIndex168 = allocator.allocate(8388608); assert(binIndex168 == 168);
    uint32_t binIndex169 = allocator.allocate(9437184); assert(binIndex169 == 169);
    uint32_t binIndex170 = allocator.allocate(10485760); assert(binIndex170 == 170);
    uint32_t binIndex171 = allocator.allocate(11534336); assert(binIndex171 == 171);
    uint32_t binIndex172 = allocator.allocate(12582912); assert(binIndex172 == 172);
    uint32_t binIndex173 = allocator.allocate(13631488); assert(binIndex173 == 173);
    uint32_t binIndex174 = allocator.allocate(14680064); assert(binIndex174 == 174);
    uint32_t binIndex175 = allocator.allocate(15728640); assert(binIndex175 == 175);
    uint32_t binIndex176 = allocator.allocate(16777216); assert(binIndex176 == 176);
    uint32_t binIndex177 = allocator.allocate(18874368); assert(binIndex177 == 177);
    uint32_t binIndex178 = allocator.allocate(20971520); assert(binIndex178 == 178);
    uint32_t binIndex179 = allocator.allocate(23068672); assert(binIndex179 == 179);
    uint32_t binIndex180 = allocator.allocate(25165824); assert(binIndex180 == 180);
    uint32_t binIndex181 = allocator.allocate(27262976); assert(binIndex181 == 181);
    uint32_t binIndex182 = allocator.allocate(29360128); assert(binIndex182 == 182);
    uint32_t binIndex183 = allocator.allocate(31457280); assert(binIndex183 == 183);
    uint32_t binIndex184 = allocator.allocate(33554432); assert(binIndex184 == 184);
    uint32_t binIndex185 = allocator.allocate(37748736); assert(binIndex185 == 185);
    uint32_t binIndex186 = allocator.allocate(41943040); assert(binIndex186 == 186);
    uint32_t binIndex187 = allocator.allocate(46137344); assert(binIndex187 == 187);
    uint32_t binIndex188 = allocator.allocate(50331648); assert(binIndex188 == 188);
    uint32_t binIndex189 = allocator.allocate(54525952); assert(binIndex189 == 189);
    uint32_t binIndex190 = allocator.allocate(58720256); assert(binIndex190 == 190);
    uint32_t binIndex191 = allocator.allocate(62914560); assert(binIndex191 == 191);
    uint32_t binIndex192 = allocator.allocate(67108864); assert(binIndex192 == 192);
    uint32_t binIndex193 = allocator.allocate(75497472); assert(binIndex193 == 193);
    uint32_t binIndex194 = allocator.allocate(83886080); assert(binIndex194 == 194);
    uint32_t binIndex195 = allocator.allocate(92274688); assert(binIndex195 == 195);
    uint32_t binIndex196 = allocator.allocate(100663296); assert(binIndex196 == 196);
    uint32_t binIndex197 = allocator.allocate(109051904); assert(binIndex197 == 197);
    uint32_t binIndex198 = allocator.allocate(117440512); assert(binIndex198 == 198);
    uint32_t binIndex199 = allocator.allocate(125829120); assert(binIndex199 == 199);
    uint32_t binIndex200 = allocator.allocate(134217728); assert(binIndex200 == 200);
    uint32_t binIndex201 = allocator.allocate(150994944); assert(binIndex201 == 201);
    uint32_t binIndex202 = allocator.allocate(167772160); assert(binIndex202 == 202);
    uint32_t binIndex203 = allocator.allocate(184549376); assert(binIndex203 == 203);
    uint32_t binIndex204 = allocator.allocate(201326592); assert(binIndex204 == 204);
    uint32_t binIndex205 = allocator.allocate(218103808); assert(binIndex205 == 205);
    uint32_t binIndex206 = allocator.allocate(234881024); assert(binIndex206 == 206);
    uint32_t binIndex207 = allocator.allocate(251658240); assert(binIndex207 == 207);
    uint32_t binIndex208 = allocator.allocate(268435456); assert(binIndex208 == 208);
    uint32_t binIndex209 = allocator.allocate(301989888); assert(binIndex209 == 209);
    uint32_t binIndex210 = allocator.allocate(335544320); assert(binIndex210 == 210);
    uint32_t binIndex211 = allocator.allocate(369098752); assert(binIndex211 == 211);
    uint32_t binIndex212 = allocator.allocate(402653184); assert(binIndex212 == 212);
    uint32_t binIndex213 = allocator.allocate(436207616); assert(binIndex213 == 213);
    uint32_t binIndex214 = allocator.allocate(469762048); assert(binIndex214 == 214);
    uint32_t binIndex215 = allocator.allocate(503316480); assert(binIndex215 == 215);
    uint32_t binIndex216 = allocator.allocate(536870912); assert(binIndex216 == 216);
    uint32_t binIndex217 = allocator.allocate(603979776); assert(binIndex217 == 217);
    uint32_t binIndex218 = allocator.allocate(671088640); assert(binIndex218 == 218);
    uint32_t binIndex219 = allocator.allocate(738197504); assert(binIndex219 == 219);
    uint32_t binIndex220 = allocator.allocate(805306368); assert(binIndex220 == 220);
    uint32_t binIndex221 = allocator.allocate(872415232); assert(binIndex221 == 221);
    uint32_t binIndex222 = allocator.allocate(939524096); assert(binIndex222 == 222);
    uint32_t binIndex223 = allocator.allocate(1006632960); assert(binIndex223 == 223);
    uint32_t binIndex224 = allocator.allocate(1073741824); assert(binIndex224 == 224);
    uint32_t binIndex225 = allocator.allocate(1207959552); assert(binIndex225 == 225);
    uint32_t binIndex226 = allocator.allocate(1342177280); assert(binIndex226 == 226);
    uint32_t binIndex227 = allocator.allocate(1476395008); assert(binIndex227 == 227);
    uint32_t binIndex228 = allocator.allocate(1610612736); assert(binIndex228 == 228);
    uint32_t binIndex229 = allocator.allocate(1744830464); assert(binIndex229 == 229);
    uint32_t binIndex230 = allocator.allocate(1879048192); assert(binIndex230 == 230);
    uint32_t binIndex231 = allocator.allocate(2013265920); assert(binIndex231 == 231);
    uint32_t binIndex232 = allocator.allocate(2147483648); assert(binIndex232 == 232);
    uint32_t binIndex233 = allocator.allocate(2415919104); assert(binIndex233 == 233);
    uint32_t binIndex234 = allocator.allocate(2684354560); assert(binIndex234 == 234);
    uint32_t binIndex235 = allocator.allocate(2952790016); assert(binIndex235 == 235);
    uint32_t binIndex236 = allocator.allocate(3221225472); assert(binIndex236 == 236);
    uint32_t binIndex237 = allocator.allocate(3489660928); assert(binIndex237 == 237);
    uint32_t binIndex238 = allocator.allocate(3758096384); assert(binIndex238 == 238);
    uint32_t binIndex239 = allocator.allocate(4026531840); assert(binIndex239 == 239);
    */

    allocator.allocate(1000);

    int x = 0;
}