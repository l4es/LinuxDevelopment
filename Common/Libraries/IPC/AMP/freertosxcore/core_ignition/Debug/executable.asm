
core_ignition.elf:     file format elf32-microblaze

Disassembly of section .vectors.reset:

00000000 <_start>:
   0:	b8080050 	brai	80	// 50 <_start1>
Disassembly of section .vectors.sw_exception:

00000008 <_vector_sw_exception>:
   8:	b80805a4 	brai	1444	// 5a4 <_exception_handler>
Disassembly of section .vectors.interrupt:

00000010 <_vector_interrupt>:
  10:	b80801b0 	brai	432	// 1b0 <_interrupt_handler>
Disassembly of section .vectors.hw_exception:

00000020 <_vector_hw_exception>:
  20:	b80805bc 	brai	1468	// 5bc <_hw_exception_handler>
Disassembly of section .text:

00000050 <_start1>:
      50:	31a01b90 	addik	r13, r0, 7056	// 1b90 <__sdata_end>
      54:	30401a38 	addik	r2, r0, 6712	// 1a38 <__sdata2_end>
      58:	302023f0 	addik	r1, r0, 9200
      5c:	b9f400c8 	brlid	r15, 200	// 124 <_crtinit>
      60:	80000000 	or	r0, r0, r0
      64:	b9f416c4 	brlid	r15, 5828	// 1728 <exit>
      68:	30a30000 	addik	r5, r3, 0

0000006c <_exit>:
      6c:	b8000000 	bri	0	// 6c <_exit>

00000070 <__do_global_dtors_aux>:
      70:	e0601b90 	lbui	r3, r0, 7056	// 1b90 <__sdata_end>
      74:	3021ffe4 	addik	r1, r1, -28
      78:	be03001c 	beqid	r3, 28		// 94
      7c:	f9e10000 	swi	r15, r1, 0
      80:	b8100048 	brid	72		// c8
      84:	e9e10000 	lwi	r15, r1, 0
      88:	f8601a40 	swi	r3, r0, 6720	// 1a40 <p.2207>
      8c:	99fc2000 	brald	r15, r4
      90:	80000000 	or	r0, r0, r0
      94:	e8601a40 	lwi	r3, r0, 6720	// 1a40 <p.2207>
      98:	e8830000 	lwi	r4, r3, 0
      9c:	be24ffec 	bneid	r4, -20		// 88
      a0:	30630004 	addik	r3, r3, 4
      a4:	b0000000 	imm	0
      a8:	30600000 	addik	r3, r0, 0
      ac:	bc030010 	beqi	r3, 16		// bc
      b0:	30a01b84 	addik	r5, r0, 7044	// 1b84 <__DTOR_END__>
      b4:	99fc1800 	brald	r15, r3
      b8:	80000000 	or	r0, r0, r0
      bc:	30600001 	addik	r3, r0, 1
      c0:	f0601b90 	sbi	r3, r0, 7056	// 1b90 <__sdata_end>
      c4:	e9e10000 	lwi	r15, r1, 0
      c8:	b60f0008 	rtsd	r15, 8
      cc:	3021001c 	addik	r1, r1, 28

000000d0 <frame_dummy>:
      d0:	b0000000 	imm	0
      d4:	30600000 	addik	r3, r0, 0
      d8:	3021ffe4 	addik	r1, r1, -28
      dc:	30a01b84 	addik	r5, r0, 7044	// 1b84 <__DTOR_END__>
      e0:	30c01b94 	addik	r6, r0, 7060	// 1b94 <object.2262>
      e4:	be030010 	beqid	r3, 16		// f4
      e8:	f9e10000 	swi	r15, r1, 0
      ec:	99fc1800 	brald	r15, r3
      f0:	80000000 	or	r0, r0, r0
      f4:	e8601b88 	lwi	r3, r0, 7048	// 1b88 <__JCR_END__>
      f8:	b0000000 	imm	0
      fc:	30800000 	addik	r4, r0, 0
     100:	be03001c 	beqid	r3, 28		// 11c
     104:	e9e10000 	lwi	r15, r1, 0
     108:	30a01b88 	addik	r5, r0, 7048	// 1b88 <__JCR_END__>
     10c:	bc040010 	beqi	r4, 16		// 11c
     110:	99fc2000 	brald	r15, r4
     114:	80000000 	or	r0, r0, r0
     118:	e9e10000 	lwi	r15, r1, 0
     11c:	b60f0008 	rtsd	r15, 8
     120:	3021001c 	addik	r1, r1, 28

00000124 <_crtinit>:
     124:	2021ffec 	addi	r1, r1, -20
     128:	f9e10000 	swi	r15, r1, 0
     12c:	20c01b90 	addi	r6, r0, 7056	// 1b90 <__sdata_end>
     130:	20e01b90 	addi	r7, r0, 7056	// 1b90 <__sdata_end>
     134:	06463800 	rsub	r18, r6, r7
     138:	bc720014 	blei	r18, 20		// 14c
     13c:	f8060000 	swi	r0, r6, 0
     140:	20c60004 	addi	r6, r6, 4
     144:	06463800 	rsub	r18, r6, r7
     148:	bc92fff4 	bgti	r18, -12		// 13c
     14c:	20c01b90 	addi	r6, r0, 7056	// 1b90 <__sdata_end>
     150:	20e01c00 	addi	r7, r0, 7168	// 1c00 <__bss_end>
     154:	06463800 	rsub	r18, r6, r7
     158:	bc720014 	blei	r18, 20		// 16c
     15c:	f8060000 	swi	r0, r6, 0
     160:	20c60004 	addi	r6, r6, 4
     164:	06463800 	rsub	r18, r6, r7
     168:	bc92fff4 	bgti	r18, -12		// 15c
     16c:	b9f40448 	brlid	r15, 1096	// 5b4 <_program_init>
     170:	80000000 	or	r0, r0, r0
     174:	b9f41800 	brlid	r15, 6144	// 1974 <__init>
     178:	80000000 	or	r0, r0, r0
     17c:	20c00000 	addi	r6, r0, 0
     180:	20e00000 	addi	r7, r0, 0
     184:	b9f40378 	brlid	r15, 888	// 4fc <main>
     188:	20a00000 	addi	r5, r0, 0
     18c:	32630000 	addik	r19, r3, 0
     190:	b9f41818 	brlid	r15, 6168	// 19a8 <__fini>
     194:	80000000 	or	r0, r0, r0
     198:	b9f40414 	brlid	r15, 1044	// 5ac <_program_clean>
     19c:	80000000 	or	r0, r0, r0
     1a0:	c9e10000 	lw	r15, r1, r0
     1a4:	30730000 	addik	r3, r19, 0
     1a8:	b60f0008 	rtsd	r15, 8
     1ac:	20210014 	addi	r1, r1, 20

000001b0 <_interrupt_handler>:
     1b0:	b0009000 	imm	-28672
     1b4:	fa400004 	swi	r18, r0, 4
     1b8:	ea401bec 	lwi	r18, r0, 7148	// 1bec <masterInterruptHandler>
     1bc:	98089000 	bra	r18

000001c0 <setupInterruptController>:
     1c0:	3021ffd8 	addik	r1, r1, -40
     1c4:	f9e10000 	swi	r15, r1, 0
     1c8:	fa610024 	swi	r19, r1, 36
     1cc:	12610000 	addk	r19, r1, r0
     1d0:	f813001c 	swi	r0, r19, 28
     1d4:	30a01bb4 	addik	r5, r0, 7092	// 1bb4 <xIntc>
     1d8:	30c00000 	addik	r6, r0, 0
     1dc:	b9f40904 	brlid	r15, 2308	// ae0 <XIntc_Initialize>
     1e0:	80000000 	or	r0, r0, r0
     1e4:	f8730020 	swi	r3, r19, 32
     1e8:	e8730020 	lwi	r3, r19, 32
     1ec:	bc23003c 	bnei	r3, 60		// 228
     1f0:	e8601bb4 	lwi	r3, r0, 7092	// 1bb4 <xIntc>
     1f4:	10a30000 	addk	r5, r3, r0
     1f8:	30c00002 	addik	r6, r0, 2
     1fc:	b9f40b74 	brlid	r15, 2932	// d70 <XIntc_SetIntrSvcOption>
     200:	80000000 	or	r0, r0, r0
     204:	30a01bb4 	addik	r5, r0, 7092	// 1bb4 <xIntc>
     208:	30c00001 	addik	r6, r0, 1
     20c:	b9f4080c 	brlid	r15, 2060	// a18 <XIntc_Start>
     210:	80000000 	or	r0, r0, r0
     214:	f8730020 	swi	r3, r19, 32
     218:	e8730020 	lwi	r3, r19, 32
     21c:	bc23000c 	bnei	r3, 12		// 228
     220:	30600001 	addik	r3, r0, 1
     224:	f873001c 	swi	r3, r19, 28
     228:	e873001c 	lwi	r3, r19, 28
     22c:	e9e10000 	lwi	r15, r1, 0
     230:	10330000 	addk	r1, r19, r0
     234:	ea610024 	lwi	r19, r1, 36
     238:	30210028 	addik	r1, r1, 40
     23c:	b60f0008 	rtsd	r15, 8
     240:	80000000 	or	r0, r0, r0

00000244 <enableTimerInterrupt>:
     244:	3021ffe0 	addik	r1, r1, -32
     248:	f9e10000 	swi	r15, r1, 0
     24c:	fa61001c 	swi	r19, r1, 28
     250:	12610000 	addk	r19, r1, r0
     254:	30a01bb4 	addik	r5, r0, 7092	// 1bb4 <xIntc>
     258:	10c00000 	addk	r6, r0, r0
     25c:	b9f40538 	brlid	r15, 1336	// 794 <XIntc_Enable>
     260:	80000000 	or	r0, r0, r0
     264:	b9f4035c 	brlid	r15, 860	// 5c0 <microblaze_enable_interrupts>
     268:	80000000 	or	r0, r0, r0
     26c:	e9e10000 	lwi	r15, r1, 0
     270:	10330000 	addk	r1, r19, r0
     274:	ea61001c 	lwi	r19, r1, 28
     278:	30210020 	addik	r1, r1, 32
     27c:	b60f0008 	rtsd	r15, 8
     280:	80000000 	or	r0, r0, r0

00000284 <connectInterruptHandler>:
     284:	3021ffd8 	addik	r1, r1, -40
     288:	f9e10000 	swi	r15, r1, 0
     28c:	fa610024 	swi	r19, r1, 36
     290:	12610000 	addk	r19, r1, r0
     294:	f813001c 	swi	r0, r19, 28
     298:	30a01bb4 	addik	r5, r0, 7092	// 1bb4 <xIntc>
     29c:	10c00000 	addk	r6, r0, r0
     2a0:	30e01360 	addik	r7, r0, 4960	// 1360 <XTmrCtr_InterruptHandler>
     2a4:	31000001 	addik	r8, r0, 1
     2a8:	b9f40628 	brlid	r15, 1576	// 8d0 <XIntc_Connect>
     2ac:	80000000 	or	r0, r0, r0
     2b0:	f8730020 	swi	r3, r19, 32
     2b4:	e8730020 	lwi	r3, r19, 32
     2b8:	bc230014 	bnei	r3, 20		// 2cc
     2bc:	b9f4ff88 	brlid	r15, -120	// 244 <enableTimerInterrupt>
     2c0:	80000000 	or	r0, r0, r0
     2c4:	30600001 	addik	r3, r0, 1
     2c8:	f873001c 	swi	r3, r19, 28
     2cc:	e873001c 	lwi	r3, r19, 28
     2d0:	e9e10000 	lwi	r15, r1, 0
     2d4:	10330000 	addk	r1, r19, r0
     2d8:	ea610024 	lwi	r19, r1, 36
     2dc:	30210028 	addik	r1, r1, 40
     2e0:	b60f0008 	rtsd	r15, 8
     2e4:	80000000 	or	r0, r0, r0

000002e8 <setupTickTimer>:
     2e8:	3021ffd8 	addik	r1, r1, -40
     2ec:	f9e10000 	swi	r15, r1, 0
     2f0:	fa610024 	swi	r19, r1, 36
     2f4:	12610000 	addk	r19, r1, r0
     2f8:	f813001c 	swi	r0, r19, 28
     2fc:	30a01bc8 	addik	r5, r0, 7112	// 1bc8 <xTickTimer>
     300:	30c00000 	addik	r6, r0, 0
     304:	b9f40f5c 	brlid	r15, 3932	// 1260 <XTmrCtr_Initialize>
     308:	80000000 	or	r0, r0, r0
     30c:	f8730020 	swi	r3, r19, 32
     310:	e8730020 	lwi	r3, r19, 32
     314:	bc23007c 	bnei	r3, 124		// 390
     318:	30a01bc8 	addik	r5, r0, 7112	// 1bc8 <xTickTimer>
     31c:	10c00000 	addk	r6, r0, r0
     320:	30e0002c 	addik	r7, r0, 44
     324:	b9f4129c 	brlid	r15, 4764	// 15c0 <XTmrCtr_SetOptions>
     328:	80000000 	or	r0, r0, r0
     32c:	30a01bc8 	addik	r5, r0, 7112	// 1bc8 <xTickTimer>
     330:	10c00000 	addk	r6, r0, r0
     334:	b0000001 	imm	1
     338:	30e0e848 	addik	r7, r0, -6072
     33c:	b9f40cb0 	brlid	r15, 3248	// fec <XTmrCtr_SetResetValue>
     340:	80000000 	or	r0, r0, r0
     344:	e8601a44 	lwi	r3, r0, 6724	// 1a44 <cpuComBlockPtr>
     348:	e863000c 	lwi	r3, r3, 12
     34c:	30a01bc8 	addik	r5, r0, 7112	// 1bc8 <xTickTimer>
     350:	10c30000 	addk	r6, r3, r0
     354:	30e00001 	addik	r7, r0, 1
     358:	b9f410f0 	brlid	r15, 4336	// 1448 <XTmrCtr_SetHandler>
     35c:	80000000 	or	r0, r0, r0
     360:	b9f4ff24 	brlid	r15, -220	// 284 <connectInterruptHandler>
     364:	80000000 	or	r0, r0, r0
     368:	f873001c 	swi	r3, r19, 28
     36c:	e873001c 	lwi	r3, r19, 28
     370:	aa430001 	xori	r18, r3, 1
     374:	bc32001c 	bnei	r18, 28		// 390
     378:	30a01bc8 	addik	r5, r0, 7112	// 1bc8 <xTickTimer>
     37c:	10c00000 	addk	r6, r0, r0
     380:	b9f40e44 	brlid	r15, 3652	// 11c4 <XTmrCtr_Start>
     384:	80000000 	or	r0, r0, r0
     388:	30600001 	addik	r3, r0, 1
     38c:	f873001c 	swi	r3, r19, 28
     390:	e873001c 	lwi	r3, r19, 28
     394:	e9e10000 	lwi	r15, r1, 0
     398:	10330000 	addk	r1, r19, r0
     39c:	ea610024 	lwi	r19, r1, 36
     3a0:	30210028 	addik	r1, r1, 40
     3a4:	b60f0008 	rtsd	r15, 8
     3a8:	80000000 	or	r0, r0, r0

000003ac <safePrint>:
     3ac:	3021ffec 	addik	r1, r1, -20
     3b0:	fa610010 	swi	r19, r1, 16
     3b4:	12610000 	addk	r19, r1, r0
     3b8:	f8b30018 	swi	r5, r19, 24
     3bc:	f813000c 	swi	r0, r19, 12
     3c0:	30600028 	addik	r3, r0, 40
     3c4:	f873000c 	swi	r3, r19, 12
     3c8:	f8130008 	swi	r0, r19, 8
     3cc:	b8000038 	bri	56		// 404
     3d0:	e8730008 	lwi	r3, r19, 8
     3d4:	10830000 	addk	r4, r3, r0
     3d8:	e8730018 	lwi	r3, r19, 24
     3dc:	10641800 	addk	r3, r4, r3
     3e0:	e0630000 	lbui	r3, r3, 0
     3e4:	90630060 	sext8	r3, r3
     3e8:	bc230010 	bnei	r3, 16		// 3f8
     3ec:	e8730008 	lwi	r3, r19, 8
     3f0:	f873000c 	swi	r3, r19, 12
     3f4:	b8000020 	bri	32		// 414
     3f8:	e8730008 	lwi	r3, r19, 8
     3fc:	30630001 	addik	r3, r3, 1
     400:	f8730008 	swi	r3, r19, 8
     404:	e8730008 	lwi	r3, r19, 8
     408:	32400027 	addik	r18, r0, 39
     40c:	16439001 	cmp	r18, r3, r18
     410:	bcb2ffc0 	bgei	r18, -64		// 3d0
     414:	e8601a44 	lwi	r3, r0, 6724	// 1a44 <cpuComBlockPtr>
     418:	e463004c 	lhui	r3, r3, 76
     41c:	90630061 	sext16	r3, r3
     420:	bc23fff4 	bnei	r3, -12		// 414
     424:	f4130004 	shi	r0, r19, 4
     428:	f8130008 	swi	r0, r19, 8
     42c:	b8000070 	bri	112		// 49c
     430:	e8730008 	lwi	r3, r19, 8
     434:	10830000 	addk	r4, r3, r0
     438:	e8730018 	lwi	r3, r19, 24
     43c:	10641800 	addk	r3, r4, r3
     440:	e0630000 	lbui	r3, r3, 0
     444:	90630060 	sext8	r3, r3
     448:	bc030014 	beqi	r3, 20		// 45c
     44c:	e4730004 	lhui	r3, r19, 4
     450:	90630061 	sext16	r3, r3
     454:	aa430001 	xori	r18, r3, 1
     458:	bc320010 	bnei	r18, 16		// 468
     45c:	30600001 	addik	r3, r0, 1
     460:	f4730004 	shi	r3, r19, 4
     464:	b800002c 	bri	44		// 490
     468:	e8a01a44 	lwi	r5, r0, 6724	// 1a44 <cpuComBlockPtr>
     46c:	e8d30008 	lwi	r6, r19, 8
     470:	e8730008 	lwi	r3, r19, 8
     474:	10830000 	addk	r4, r3, r0
     478:	e8730018 	lwi	r3, r19, 24
     47c:	10641800 	addk	r3, r4, r3
     480:	e0630000 	lbui	r3, r3, 0
     484:	90830060 	sext8	r4, r3
     488:	10662800 	addk	r3, r6, r5
     48c:	f0830024 	sbi	r4, r3, 36
     490:	e8730008 	lwi	r3, r19, 8
     494:	30630001 	addik	r3, r3, 1
     498:	f8730008 	swi	r3, r19, 8
     49c:	e8730008 	lwi	r3, r19, 8
     4a0:	32400027 	addik	r18, r0, 39
     4a4:	16439001 	cmp	r18, r3, r18
     4a8:	bcb2ff88 	bgei	r18, -120		// 430
     4ac:	e8601a44 	lwi	r3, r0, 6724	// 1a44 <cpuComBlockPtr>
     4b0:	e893000c 	lwi	r4, r19, 12
     4b4:	90840061 	sext16	r4, r4
     4b8:	f483004c 	shi	r4, r3, 76
     4bc:	10330000 	addk	r1, r19, r0
     4c0:	ea610010 	lwi	r19, r1, 16
     4c4:	30210014 	addik	r1, r1, 20
     4c8:	b60f0008 	rtsd	r15, 8
     4cc:	80000000 	or	r0, r0, r0

000004d0 <quickDebug>:
     4d0:	3021fff8 	addik	r1, r1, -8
     4d4:	fa610004 	swi	r19, r1, 4
     4d8:	12610000 	addk	r19, r1, r0
     4dc:	e8601bb0 	lwi	r3, r0, 7088	// 1bb0 <worker>
     4e0:	30630001 	addik	r3, r3, 1
     4e4:	f8601bb0 	swi	r3, r0, 7088	// 1bb0 <worker>
     4e8:	10330000 	addk	r1, r19, r0
     4ec:	ea610004 	lwi	r19, r1, 4
     4f0:	30210008 	addik	r1, r1, 8
     4f4:	b60f0008 	rtsd	r15, 8
     4f8:	80000000 	or	r0, r0, r0

000004fc <main>:
     4fc:	3021ffd8 	addik	r1, r1, -40
     500:	f9e10000 	swi	r15, r1, 0
     504:	fa610024 	swi	r19, r1, 36
     508:	12610000 	addk	r19, r1, r0
     50c:	e8801a44 	lwi	r4, r0, 6724	// 1a44 <cpuComBlockPtr>
     510:	b0000000 	imm	0
     514:	3060db0e 	addik	r3, r0, -9458
     518:	f8640008 	swi	r3, r4, 8
     51c:	e8601a44 	lwi	r3, r0, 6724	// 1a44 <cpuComBlockPtr>
     520:	e8630008 	lwi	r3, r3, 8
     524:	aa432f02 	xori	r18, r3, 12034
     528:	bc32fff4 	bnei	r18, -12		// 51c
     52c:	e8801a44 	lwi	r4, r0, 6724	// 1a44 <cpuComBlockPtr>
     530:	b0000000 	imm	0
     534:	3060db0e 	addik	r3, r0, -9458
     538:	f8640008 	swi	r3, r4, 8
     53c:	e8b30020 	lwi	r5, r19, 32
     540:	b9f400a0 	brlid	r15, 160	// 5e0 <microblaze_get_pvr>
     544:	80000000 	or	r0, r0, r0
     548:	e8730020 	lwi	r3, r19, 32
     54c:	e8630000 	lwi	r3, r3, 0
     550:	f073001c 	sbi	r3, r19, 28
     554:	e8601a44 	lwi	r3, r0, 6724	// 1a44 <cpuComBlockPtr>
     558:	e8630010 	lwi	r3, r3, 16
     55c:	f8601bec 	swi	r3, r0, 7148	// 1bec <masterInterruptHandler>
     560:	e8601a44 	lwi	r3, r0, 6724	// 1a44 <cpuComBlockPtr>
     564:	e8630014 	lwi	r3, r3, 20
     568:	f8601be4 	swi	r3, r0, 7140	// 1be4 <taskLauncher>
     56c:	b9f4fc54 	brlid	r15, -940	// 1c0 <setupInterruptController>
     570:	80000000 	or	r0, r0, r0
     574:	b9f4fd74 	brlid	r15, -652	// 2e8 <setupTickTimer>
     578:	80000000 	or	r0, r0, r0
     57c:	e8601be4 	lwi	r3, r0, 7140	// 1be4 <taskLauncher>
     580:	99fc1800 	brald	r15, r3
     584:	80000000 	or	r0, r0, r0
     588:	10600000 	addk	r3, r0, r0
     58c:	e9e10000 	lwi	r15, r1, 0
     590:	10330000 	addk	r1, r19, r0
     594:	ea610024 	lwi	r19, r1, 36
     598:	30210028 	addik	r1, r1, 40
     59c:	b60f0008 	rtsd	r15, 8
     5a0:	80000000 	or	r0, r0, r0

000005a4 <_exception_handler>:
     5a4:	b6110000 	rtsd	r17, 0
     5a8:	80000000 	or	r0, r0, r0

000005ac <_program_clean>:
     5ac:	b60f0008 	rtsd	r15, 8
     5b0:	80000000 	or	r0, r0, r0

000005b4 <_program_init>:
     5b4:	b60f0008 	rtsd	r15, 8
     5b8:	80000000 	or	r0, r0, r0

000005bc <_hw_exception_handler>:
     5bc:	b8000000 	bri	0	// 5bc <_hw_exception_handler>

000005c0 <microblaze_enable_interrupts>:
     5c0:	2021fffc 	addi	r1, r1, -4
     5c4:	f9810000 	swi	r12, r1, 0
     5c8:	95808001 	mfs	r12, rmsr
     5cc:	a18c0002 	ori	r12, r12, 2
     5d0:	940cc001 	mts	rmsr, r12
     5d4:	e9810000 	lwi	r12, r1, 0
     5d8:	b60f0008 	rtsd	r15, 8
     5dc:	20210004 	addi	r1, r1, 4

000005e0 <microblaze_get_pvr>:
     5e0:	be050018 	beqid	r5, 24		// 5f8
     5e4:	3060ffff 	addik	r3, r0, -1
     5e8:	f8050000 	swi	r0, r5, 0
     5ec:	9460a000 	mfs	r3, rpvr0
     5f0:	f8650000 	swi	r3, r5, 0
     5f4:	10600000 	addk	r3, r0, r0
     5f8:	b60f0008 	rtsd	r15, 8
     5fc:	80000000 	or	r0, r0, r0

00000600 <XIntc_LookupConfig>:
     600:	e4601a48 	lhui	r3, r0, 6728	// 1a48 <XIntc_ConfigTable>
     604:	b0000000 	imm	0
     608:	a4a5ffff 	andi	r5, r5, -1
     60c:	30801a48 	addik	r4, r0, 6728	// 1a48 <XIntc_ConfigTable>
     610:	16451800 	rsubk	r18, r5, r3
     614:	bc32000c 	bnei	r18, 12		// 620
     618:	b60f0008 	rtsd	r15, 8
     61c:	10640000 	addk	r3, r4, r0
     620:	10800000 	addk	r4, r0, r0
     624:	b60f0008 	rtsd	r15, 8
     628:	10640000 	addk	r3, r4, r0

0000062c <StubHandler>:
     62c:	10850000 	addk	r4, r5, r0
     630:	3021ffe4 	addik	r1, r1, -28
     634:	f9e10000 	swi	r15, r1, 0
     638:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     63c:	be040024 	beqid	r4, 36		// 660
     640:	30c00231 	addik	r6, r0, 561
     644:	e864000c 	lwi	r3, r4, 12
     648:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     64c:	30630001 	addik	r3, r3, 1
     650:	f864000c 	swi	r3, r4, 12
     654:	e9e10000 	lwi	r15, r1, 0
     658:	b60f0008 	rtsd	r15, 8
     65c:	3021001c 	addik	r1, r1, 28
     660:	b9f41090 	brlid	r15, 4240	// 16f0 <Xil_Assert>
     664:	80000000 	or	r0, r0, r0
     668:	30600001 	addik	r3, r0, 1
     66c:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     670:	b810ffe8 	brid	-24		// 658
     674:	e9e10000 	lwi	r15, r1, 0

00000678 <XIntc_Acknowledge>:
     678:	10850000 	addk	r4, r5, r0
     67c:	3021ffe4 	addik	r1, r1, -28
     680:	a4e600ff 	andi	r7, r6, 255
     684:	f9e10000 	swi	r15, r1, 0
     688:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     68c:	be04001c 	beqid	r4, 28		// 6a8
     690:	30c0020d 	addik	r6, r0, 525
     694:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     698:	22400001 	addi	r18, r0, 1
     69c:	16479003 	cmpu	r18, r7, r18
     6a0:	beb20024 	bgeid	r18, 36		// 6c4
     6a4:	30c0020e 	addik	r6, r0, 526
     6a8:	b9f41048 	brlid	r15, 4168	// 16f0 <Xil_Assert>
     6ac:	80000000 	or	r0, r0, r0
     6b0:	e9e10000 	lwi	r15, r1, 0
     6b4:	30600001 	addik	r3, r0, 1
     6b8:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     6bc:	b60f0008 	rtsd	r15, 8
     6c0:	3021001c 	addik	r1, r1, 28
     6c4:	e8640004 	lwi	r3, r4, 4
     6c8:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     6cc:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     6d0:	b0001111 	imm	4369
     6d4:	aa431111 	xori	r18, r3, 4369
     6d8:	be32ffd0 	bneid	r18, -48		// 6a8
     6dc:	30c0020f 	addik	r6, r0, 527
     6e0:	64670402 	bslli	r3, r7, 2
     6e4:	e8a40000 	lwi	r5, r4, 0
     6e8:	e8831bf0 	lwi	r4, r3, 7152
     6ec:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     6f0:	f885000c 	swi	r4, r5, 12
     6f4:	e9e10000 	lwi	r15, r1, 0
     6f8:	b60f0008 	rtsd	r15, 8
     6fc:	3021001c 	addik	r1, r1, 28

00000700 <XIntc_Disable>:
     700:	10850000 	addk	r4, r5, r0
     704:	3021ffe4 	addik	r1, r1, -28
     708:	a4e600ff 	andi	r7, r6, 255
     70c:	f9e10000 	swi	r15, r1, 0
     710:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     714:	be04001c 	beqid	r4, 28		// 730
     718:	30c001e2 	addik	r6, r0, 482
     71c:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     720:	22400001 	addi	r18, r0, 1
     724:	16479003 	cmpu	r18, r7, r18
     728:	beb20024 	bgeid	r18, 36		// 74c
     72c:	30c001e3 	addik	r6, r0, 483
     730:	b9f40fc0 	brlid	r15, 4032	// 16f0 <Xil_Assert>
     734:	80000000 	or	r0, r0, r0
     738:	e9e10000 	lwi	r15, r1, 0
     73c:	30600001 	addik	r3, r0, 1
     740:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     744:	b60f0008 	rtsd	r15, 8
     748:	3021001c 	addik	r1, r1, 28
     74c:	e8640004 	lwi	r3, r4, 4
     750:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     754:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     758:	b0001111 	imm	4369
     75c:	aa431111 	xori	r18, r3, 4369
     760:	be32ffd0 	bneid	r18, -48		// 730
     764:	30c001e4 	addik	r6, r0, 484
     768:	e8c40000 	lwi	r6, r4, 0
     76c:	64a70402 	bslli	r5, r7, 2
     770:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     774:	e8651bf0 	lwi	r3, r5, 7152
     778:	e8860008 	lwi	r4, r6, 8
     77c:	a863ffff 	xori	r3, r3, -1
     780:	84841800 	and	r4, r4, r3
     784:	f8860008 	swi	r4, r6, 8
     788:	e9e10000 	lwi	r15, r1, 0
     78c:	b60f0008 	rtsd	r15, 8
     790:	3021001c 	addik	r1, r1, 28

00000794 <XIntc_Enable>:
     794:	10e50000 	addk	r7, r5, r0
     798:	3021ffe4 	addik	r1, r1, -28
     79c:	a48600ff 	andi	r4, r6, 255
     7a0:	f9e10000 	swi	r15, r1, 0
     7a4:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     7a8:	be07001c 	beqid	r7, 28		// 7c4
     7ac:	30c001b5 	addik	r6, r0, 437
     7b0:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     7b4:	22400001 	addi	r18, r0, 1
     7b8:	16449003 	cmpu	r18, r4, r18
     7bc:	beb20024 	bgeid	r18, 36		// 7e0
     7c0:	30c001b6 	addik	r6, r0, 438
     7c4:	b9f40f2c 	brlid	r15, 3884	// 16f0 <Xil_Assert>
     7c8:	80000000 	or	r0, r0, r0
     7cc:	e9e10000 	lwi	r15, r1, 0
     7d0:	30600001 	addik	r3, r0, 1
     7d4:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     7d8:	b60f0008 	rtsd	r15, 8
     7dc:	3021001c 	addik	r1, r1, 28
     7e0:	e8670004 	lwi	r3, r7, 4
     7e4:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     7e8:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     7ec:	b0001111 	imm	4369
     7f0:	aa431111 	xori	r18, r3, 4369
     7f4:	be32ffd0 	bneid	r18, -48		// 7c4
     7f8:	30c001b7 	addik	r6, r0, 439
     7fc:	e8c70000 	lwi	r6, r7, 0
     800:	64840402 	bslli	r4, r4, 2
     804:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     808:	e8a41bf0 	lwi	r5, r4, 7152
     80c:	e8660008 	lwi	r3, r6, 8
     810:	80632800 	or	r3, r3, r5
     814:	f8660008 	swi	r3, r6, 8
     818:	e9e10000 	lwi	r15, r1, 0
     81c:	b60f0008 	rtsd	r15, 8
     820:	3021001c 	addik	r1, r1, 28

00000824 <XIntc_Disconnect>:
     824:	11250000 	addk	r9, r5, r0
     828:	3021ffe4 	addik	r1, r1, -28
     82c:	a50600ff 	andi	r8, r6, 255
     830:	f9e10000 	swi	r15, r1, 0
     834:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     838:	be09001c 	beqid	r9, 28		// 854
     83c:	30c00183 	addik	r6, r0, 387
     840:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     844:	22400001 	addi	r18, r0, 1
     848:	16489003 	cmpu	r18, r8, r18
     84c:	beb20024 	bgeid	r18, 36		// 870
     850:	30c00184 	addik	r6, r0, 388
     854:	b9f40e9c 	brlid	r15, 3740	// 16f0 <Xil_Assert>
     858:	80000000 	or	r0, r0, r0
     85c:	e9e10000 	lwi	r15, r1, 0
     860:	30600001 	addik	r3, r0, 1
     864:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     868:	b60f0008 	rtsd	r15, 8
     86c:	3021001c 	addik	r1, r1, 28
     870:	e8690004 	lwi	r3, r9, 4
     874:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     878:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     87c:	b0001111 	imm	4369
     880:	aa431111 	xori	r18, r3, 4369
     884:	be32ffd0 	bneid	r18, -48		// 854
     888:	30c00185 	addik	r6, r0, 389
     88c:	e8e90000 	lwi	r7, r9, 0
     890:	64a80402 	bslli	r5, r8, 2
     894:	e8c90010 	lwi	r6, r9, 16
     898:	e8651bf0 	lwi	r3, r5, 7152
     89c:	e8870008 	lwi	r4, r7, 8
     8a0:	64a80403 	bslli	r5, r8, 3
     8a4:	a863ffff 	xori	r3, r3, -1
     8a8:	84841800 	and	r4, r4, r3
     8ac:	3060062c 	addik	r3, r0, 1580	// 62c <StubHandler>
     8b0:	10a53000 	addk	r5, r5, r6
     8b4:	f8870008 	swi	r4, r7, 8
     8b8:	f9250014 	swi	r9, r5, 20
     8bc:	f8650010 	swi	r3, r5, 16
     8c0:	e9e10000 	lwi	r15, r1, 0
     8c4:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     8c8:	b60f0008 	rtsd	r15, 8
     8cc:	3021001c 	addik	r1, r1, 28

000008d0 <XIntc_Connect>:
     8d0:	3021ffe4 	addik	r1, r1, -28
     8d4:	f9e10000 	swi	r15, r1, 0
     8d8:	be0500a0 	beqid	r5, 160		// 978
     8dc:	a4c600ff 	andi	r6, r6, 255
     8e0:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     8e4:	22400001 	addi	r18, r0, 1
     8e8:	16469003 	cmpu	r18, r6, r18
     8ec:	bcb20028 	bgei	r18, 40		// 914
     8f0:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     8f4:	b9f40dfc 	brlid	r15, 3580	// 16f0 <Xil_Assert>
     8f8:	30c0015c 	addik	r6, r0, 348
     8fc:	30600001 	addik	r3, r0, 1
     900:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     904:	e9e10000 	lwi	r15, r1, 0
     908:	10600000 	addk	r3, r0, r0
     90c:	b60f0008 	rtsd	r15, 8
     910:	3021001c 	addik	r1, r1, 28
     914:	bc070080 	beqi	r7, 128		// 994
     918:	e8650004 	lwi	r3, r5, 4
     91c:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     920:	b0001111 	imm	4369
     924:	aa431111 	xori	r18, r3, 4369
     928:	be12002c 	beqid	r18, 44		// 954
     92c:	64660403 	bslli	r3, r6, 3
     930:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     934:	b9f40dbc 	brlid	r15, 3516	// 16f0 <Xil_Assert>
     938:	30c0015e 	addik	r6, r0, 350
     93c:	e9e10000 	lwi	r15, r1, 0
     940:	30600001 	addik	r3, r0, 1
     944:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     948:	10600000 	addk	r3, r0, r0
     94c:	b60f0008 	rtsd	r15, 8
     950:	3021001c 	addik	r1, r1, 28
     954:	e8850010 	lwi	r4, r5, 16
     958:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     95c:	10632000 	addk	r3, r3, r4
     960:	f9030014 	swi	r8, r3, 20
     964:	f8e30010 	swi	r7, r3, 16
     968:	e9e10000 	lwi	r15, r1, 0
     96c:	10600000 	addk	r3, r0, r0
     970:	b60f0008 	rtsd	r15, 8
     974:	3021001c 	addik	r1, r1, 28
     978:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     97c:	b9f40d74 	brlid	r15, 3444	// 16f0 <Xil_Assert>
     980:	30c0015b 	addik	r6, r0, 347
     984:	30600001 	addik	r3, r0, 1
     988:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     98c:	b810ff7c 	brid	-132		// 908
     990:	e9e10000 	lwi	r15, r1, 0
     994:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     998:	b9f40d58 	brlid	r15, 3416	// 16f0 <Xil_Assert>
     99c:	30c0015d 	addik	r6, r0, 349
     9a0:	30600001 	addik	r3, r0, 1
     9a4:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     9a8:	b810ff60 	brid	-160		// 908
     9ac:	e9e10000 	lwi	r15, r1, 0

000009b0 <XIntc_Stop>:
     9b0:	10850000 	addk	r4, r5, r0
     9b4:	3021ffe4 	addik	r1, r1, -28
     9b8:	f9e10000 	swi	r15, r1, 0
     9bc:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     9c0:	be040020 	beqid	r4, 32		// 9e0
     9c4:	30c0012f 	addik	r6, r0, 303
     9c8:	e8640004 	lwi	r3, r4, 4
     9cc:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     9d0:	b0001111 	imm	4369
     9d4:	aa431111 	xori	r18, r3, 4369
     9d8:	be120024 	beqid	r18, 36		// 9fc
     9dc:	30c00130 	addik	r6, r0, 304
     9e0:	b9f40d10 	brlid	r15, 3344	// 16f0 <Xil_Assert>
     9e4:	80000000 	or	r0, r0, r0
     9e8:	e9e10000 	lwi	r15, r1, 0
     9ec:	30600001 	addik	r3, r0, 1
     9f0:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     9f4:	b60f0008 	rtsd	r15, 8
     9f8:	3021001c 	addik	r1, r1, 28
     9fc:	e8640000 	lwi	r3, r4, 0
     a00:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     a04:	f803001c 	swi	r0, r3, 28
     a08:	f8040008 	swi	r0, r4, 8
     a0c:	e9e10000 	lwi	r15, r1, 0
     a10:	b60f0008 	rtsd	r15, 8
     a14:	3021001c 	addik	r1, r1, 28

00000a18 <XIntc_Start>:
     a18:	3021ffe4 	addik	r1, r1, -28
     a1c:	f9e10000 	swi	r15, r1, 0
     a20:	be0500a4 	beqid	r5, 164		// ac4
     a24:	a4c600ff 	andi	r6, r6, 255
     a28:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     a2c:	22400001 	addi	r18, r0, 1
     a30:	16469003 	cmpu	r18, r6, r18
     a34:	bcb20028 	bgei	r18, 40		// a5c
     a38:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     a3c:	b9f40cb4 	brlid	r15, 3252	// 16f0 <Xil_Assert>
     a40:	30c00103 	addik	r6, r0, 259
     a44:	30600001 	addik	r3, r0, 1
     a48:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     a4c:	e9e10000 	lwi	r15, r1, 0
     a50:	10600000 	addk	r3, r0, r0
     a54:	b60f0008 	rtsd	r15, 8
     a58:	3021001c 	addik	r1, r1, 28
     a5c:	e8650004 	lwi	r3, r5, 4
     a60:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     a64:	b0001111 	imm	4369
     a68:	aa431111 	xori	r18, r3, 4369
     a6c:	bc120028 	beqi	r18, 40		// a94
     a70:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     a74:	b9f40c7c 	brlid	r15, 3196	// 16f0 <Xil_Assert>
     a78:	30c00104 	addik	r6, r0, 260
     a7c:	e9e10000 	lwi	r15, r1, 0
     a80:	30600001 	addik	r3, r0, 1
     a84:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     a88:	10600000 	addk	r3, r0, r0
     a8c:	b60f0008 	rtsd	r15, 8
     a90:	3021001c 	addik	r1, r1, 28
     a94:	be26000c 	bneid	r6, 12		// aa0
     a98:	30e00003 	addik	r7, r0, 3
     a9c:	30e00001 	addik	r7, r0, 1
     aa0:	b0002222 	imm	8738
     aa4:	30602222 	addik	r3, r0, 8738
     aa8:	e8850000 	lwi	r4, r5, 0
     aac:	f8650008 	swi	r3, r5, 8
     ab0:	f8e4001c 	swi	r7, r4, 28
     ab4:	e9e10000 	lwi	r15, r1, 0
     ab8:	10600000 	addk	r3, r0, r0
     abc:	b60f0008 	rtsd	r15, 8
     ac0:	3021001c 	addik	r1, r1, 28
     ac4:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     ac8:	b9f40c28 	brlid	r15, 3112	// 16f0 <Xil_Assert>
     acc:	30c00101 	addik	r6, r0, 257
     ad0:	30600001 	addik	r3, r0, 1
     ad4:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     ad8:	b810ff78 	brid	-136		// a50
     adc:	e9e10000 	lwi	r15, r1, 0

00000ae0 <XIntc_Initialize>:
     ae0:	3021ffe0 	addik	r1, r1, -32
     ae4:	fa61001c 	swi	r19, r1, 28
     ae8:	12650000 	addk	r19, r5, r0
     aec:	b0000000 	imm	0
     af0:	a4a6ffff 	andi	r5, r6, -1
     af4:	be1300dc 	beqid	r19, 220		// bd0
     af8:	f9e10000 	swi	r15, r1, 0
     afc:	e8730008 	lwi	r3, r19, 8
     b00:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     b04:	b0002222 	imm	8738
     b08:	aa432222 	xori	r18, r3, 8738
     b0c:	be120098 	beqid	r18, 152		// ba4
     b10:	30e00005 	addik	r7, r0, 5
     b14:	b9f4faec 	brlid	r15, -1300	// 600 <XIntc_LookupConfig>
     b18:	80000000 	or	r0, r0, r0
     b1c:	10830000 	addk	r4, r3, r0
     b20:	be030084 	beqid	r3, 132		// ba4
     b24:	30e00002 	addik	r7, r0, 2
     b28:	f8130004 	swi	r0, r19, 4
     b2c:	f8130008 	swi	r0, r19, 8
     b30:	e8c30004 	lwi	r6, r3, 4
     b34:	e8a30010 	lwi	r5, r3, 16
     b38:	30600001 	addik	r3, r0, 1
     b3c:	f864000c 	swi	r3, r4, 12
     b40:	f8930010 	swi	r4, r19, 16
     b44:	be050074 	beqid	r5, 116		// bb8
     b48:	f8d30000 	swi	r6, r19, 0
     b4c:	aa4516e8 	xori	r18, r5, 5864
     b50:	bc120068 	beqi	r18, 104		// bb8
     b54:	e8a40018 	lwi	r5, r4, 24
     b58:	30600001 	addik	r3, r0, 1
     b5c:	f8601bf0 	swi	r3, r0, 7152	// 1bf0 <XIntc_BitPosMask>
     b60:	be050064 	beqid	r5, 100		// bc4
     b64:	fa640014 	swi	r19, r4, 20
     b68:	aa4516e8 	xori	r18, r5, 5864
     b6c:	bc120058 	beqi	r18, 88		// bc4
     b70:	30600002 	addik	r3, r0, 2
     b74:	f8601bf4 	swi	r3, r0, 7156
     b78:	f806001c 	swi	r0, r6, 28
     b7c:	e8730000 	lwi	r3, r19, 0
     b80:	fa64001c 	swi	r19, r4, 28
     b84:	b0001111 	imm	4369
     b88:	30801111 	addik	r4, r0, 4369
     b8c:	f8030008 	swi	r0, r3, 8
     b90:	e8b30000 	lwi	r5, r19, 0
     b94:	3060ffff 	addik	r3, r0, -1
     b98:	10e00000 	addk	r7, r0, r0
     b9c:	f865000c 	swi	r3, r5, 12
     ba0:	f8930004 	swi	r4, r19, 4
     ba4:	10670000 	addk	r3, r7, r0
     ba8:	e9e10000 	lwi	r15, r1, 0
     bac:	ea61001c 	lwi	r19, r1, 28
     bb0:	b60f0008 	rtsd	r15, 8
     bb4:	30210020 	addik	r1, r1, 32
     bb8:	3060062c 	addik	r3, r0, 1580	// 62c <StubHandler>
     bbc:	b810ff98 	brid	-104		// b54
     bc0:	f8640010 	swi	r3, r4, 16
     bc4:	3060062c 	addik	r3, r0, 1580	// 62c <StubHandler>
     bc8:	b810ffa8 	brid	-88		// b70
     bcc:	f8640018 	swi	r3, r4, 24
     bd0:	30a019c4 	addik	r5, r0, 6596	// 19c4 <__rodata_start>
     bd4:	b9f40b1c 	brlid	r15, 2844	// 16f0 <Xil_Assert>
     bd8:	30c00086 	addik	r6, r0, 134
     bdc:	30600001 	addik	r3, r0, 1
     be0:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     be4:	b810ffc0 	brid	-64		// ba4
     be8:	10f30000 	addk	r7, r19, r0

00000bec <XIntc_DeviceInterruptHandler>:
     bec:	30601a48 	addik	r3, r0, 6728	// 1a48 <XIntc_ConfigTable>
     bf0:	64a50405 	bslli	r5, r5, 5
     bf4:	3021ffcc 	addik	r1, r1, -52
     bf8:	fa610020 	swi	r19, r1, 32
     bfc:	fac10024 	swi	r22, r1, 36
     c00:	fae10028 	swi	r23, r1, 40
     c04:	fb01002c 	swi	r24, r1, 44
     c08:	fb210030 	swi	r25, r1, 48
     c0c:	f9e10000 	swi	r15, r1, 0
     c10:	12651800 	addk	r19, r5, r3
     c14:	e8d30004 	lwi	r6, r19, 4
     c18:	33000001 	addik	r24, r0, 1
     c1c:	13200000 	addk	r25, r0, r0
     c20:	e8860000 	lwi	r4, r6, 0
     c24:	e8660008 	lwi	r3, r6, 8
     c28:	32e00010 	addik	r23, r0, 16	// 10 <_vector_interrupt>
     c2c:	86c41800 	and	r22, r4, r3
     c30:	a4760001 	andi	r3, r22, 1
     c34:	1093b800 	addk	r4, r19, r23
     c38:	33390001 	addik	r25, r25, 1
     c3c:	be030054 	beqid	r3, 84		// c90
     c40:	92d60041 	srl	r22, r22
     c44:	e8730008 	lwi	r3, r19, 8
     c48:	84781800 	and	r3, r24, r3
     c4c:	bc230088 	bnei	r3, 136		// cd4
     c50:	e8a40004 	lwi	r5, r4, 4
     c54:	c893b800 	lw	r4, r19, r23
     c58:	99fc2000 	brald	r15, r4
     c5c:	80000000 	or	r0, r0, r0
     c60:	e8730008 	lwi	r3, r19, 8
     c64:	84781800 	and	r3, r24, r3
     c68:	bc03005c 	beqi	r3, 92		// cc4
     c6c:	e8d30004 	lwi	r6, r19, 4
     c70:	e873000c 	lwi	r3, r19, 12
     c74:	e8860000 	lwi	r4, r6, 0
     c78:	e8a60008 	lwi	r5, r6, 8
     c7c:	84842800 	and	r4, r4, r5
     c80:	f881001c 	swi	r4, r1, 28
     c84:	aa430001 	xori	r18, r3, 1
     c88:	be120020 	beqid	r18, 32		// ca8
     c8c:	e9e10000 	lwi	r15, r1, 0
     c90:	be160014 	beqid	r22, 20		// ca4
     c94:	1318c000 	addk	r24, r24, r24
     c98:	aa590002 	xori	r18, r25, 2
     c9c:	be32ff94 	bneid	r18, -108		// c30
     ca0:	32f70008 	addik	r23, r23, 8
     ca4:	e9e10000 	lwi	r15, r1, 0
     ca8:	ea610020 	lwi	r19, r1, 32
     cac:	eac10024 	lwi	r22, r1, 36
     cb0:	eae10028 	lwi	r23, r1, 40
     cb4:	eb01002c 	lwi	r24, r1, 44
     cb8:	eb210030 	lwi	r25, r1, 48
     cbc:	b60f0008 	rtsd	r15, 8
     cc0:	30210034 	addik	r1, r1, 52
     cc4:	e8730004 	lwi	r3, r19, 4
     cc8:	fb03000c 	swi	r24, r3, 12
     ccc:	b810ffa4 	brid	-92		// c70
     cd0:	e8d30004 	lwi	r6, r19, 4
     cd4:	fb06000c 	swi	r24, r6, 12
     cd8:	b810ff7c 	brid	-132		// c54
     cdc:	e8a40004 	lwi	r5, r4, 4

00000ce0 <XIntc_LowLevelInterruptHandler>:
     ce0:	3021ffe4 	addik	r1, r1, -28
     ce4:	f9e10000 	swi	r15, r1, 0
     ce8:	b9f4ff04 	brlid	r15, -252	// bec <XIntc_DeviceInterruptHandler>
     cec:	10a00000 	addk	r5, r0, r0
     cf0:	e9e10000 	lwi	r15, r1, 0
     cf4:	b60f0008 	rtsd	r15, 8
     cf8:	3021001c 	addik	r1, r1, 28

00000cfc <LookupConfigByBaseAddress>:
     cfc:	e8601a4c 	lwi	r3, r0, 6732
     d00:	30801a48 	addik	r4, r0, 6728	// 1a48 <XIntc_ConfigTable>
     d04:	16451800 	rsubk	r18, r5, r3
     d08:	bc32000c 	bnei	r18, 12		// d14
     d0c:	b60f0008 	rtsd	r15, 8
     d10:	10640000 	addk	r3, r4, r0
     d14:	10800000 	addk	r4, r0, r0
     d18:	b60f0008 	rtsd	r15, 8
     d1c:	10640000 	addk	r3, r4, r0

00000d20 <XIntc_RegisterHandler>:
     d20:	3021ffd8 	addik	r1, r1, -40
     d24:	fac10020 	swi	r22, r1, 32
     d28:	12c60000 	addk	r22, r6, r0
     d2c:	fa61001c 	swi	r19, r1, 28
     d30:	fae10024 	swi	r23, r1, 36
     d34:	66d60403 	bslli	r22, r22, 3
     d38:	f9e10000 	swi	r15, r1, 0
     d3c:	12e70000 	addk	r23, r7, r0
     d40:	b9f4ffbc 	brlid	r15, -68	// cfc <LookupConfigByBaseAddress>
     d44:	12680000 	addk	r19, r8, r0
     d48:	be030010 	beqid	r3, 16		// d58
     d4c:	12d61800 	addk	r22, r22, r3
     d50:	fa760014 	swi	r19, r22, 20
     d54:	faf60010 	swi	r23, r22, 16
     d58:	e9e10000 	lwi	r15, r1, 0
     d5c:	ea61001c 	lwi	r19, r1, 28
     d60:	eac10020 	lwi	r22, r1, 32
     d64:	eae10024 	lwi	r23, r1, 36
     d68:	b60f0008 	rtsd	r15, 8
     d6c:	30210028 	addik	r1, r1, 40

00000d70 <XIntc_SetIntrSvcOption>:
     d70:	3021ffe0 	addik	r1, r1, -32
     d74:	fa61001c 	swi	r19, r1, 28
     d78:	f9e10000 	swi	r15, r1, 0
     d7c:	b9f4ff80 	brlid	r15, -128	// cfc <LookupConfigByBaseAddress>
     d80:	12660000 	addk	r19, r6, r0
     d84:	be030010 	beqid	r3, 16		// d94
     d88:	e9e10000 	lwi	r15, r1, 0
     d8c:	fa63000c 	swi	r19, r3, 12
     d90:	e9e10000 	lwi	r15, r1, 0
     d94:	ea61001c 	lwi	r19, r1, 28
     d98:	b60f0008 	rtsd	r15, 8
     d9c:	30210020 	addik	r1, r1, 32

00000da0 <XTmrCtr_LookupConfig>:
     da0:	e4601a68 	lhui	r3, r0, 6760	// 1a68 <XTmrCtr_ConfigTable>
     da4:	b0000000 	imm	0
     da8:	a4a5ffff 	andi	r5, r5, -1
     dac:	30801a68 	addik	r4, r0, 6760	// 1a68 <XTmrCtr_ConfigTable>
     db0:	16451800 	rsubk	r18, r5, r3
     db4:	bc32000c 	bnei	r18, 12		// dc0
     db8:	b60f0008 	rtsd	r15, 8
     dbc:	10640000 	addk	r3, r4, r0
     dc0:	10800000 	addk	r4, r0, r0
     dc4:	b60f0008 	rtsd	r15, 8
     dc8:	10640000 	addk	r3, r4, r0

00000dcc <XTmrCtr_IsExpired>:
     dcc:	3021ffe0 	addik	r1, r1, -32
     dd0:	fa61001c 	swi	r19, r1, 28
     dd4:	12650000 	addk	r19, r5, r0
     dd8:	a48600ff 	andi	r4, r6, 255
     ddc:	f9e10000 	swi	r15, r1, 0
     de0:	30a019cc 	addik	r5, r0, 6604
     de4:	be130094 	beqid	r19, 148		// e78
     de8:	30c001bd 	addik	r6, r0, 445
     dec:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     df0:	22400001 	addi	r18, r0, 1
     df4:	16449003 	cmpu	r18, r4, r18
     df8:	beb20030 	bgeid	r18, 48		// e28
     dfc:	30c001be 	addik	r6, r0, 446
     e00:	b9f408f0 	brlid	r15, 2288	// 16f0 <Xil_Assert>
     e04:	80000000 	or	r0, r0, r0
     e08:	30600001 	addik	r3, r0, 1
     e0c:	10a00000 	addk	r5, r0, r0
     e10:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     e14:	10650000 	addk	r3, r5, r0
     e18:	e9e10000 	lwi	r15, r1, 0
     e1c:	ea61001c 	lwi	r19, r1, 28
     e20:	b60f0008 	rtsd	r15, 8
     e24:	30210020 	addik	r1, r1, 32
     e28:	e8730008 	lwi	r3, r19, 8
     e2c:	30a019cc 	addik	r5, r0, 6604
     e30:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     e34:	b0001111 	imm	4369
     e38:	aa431111 	xori	r18, r3, 4369
     e3c:	be120020 	beqid	r18, 32		// e5c
     e40:	30c001bf 	addik	r6, r0, 447
     e44:	b9f408ac 	brlid	r15, 2220	// 16f0 <Xil_Assert>
     e48:	80000000 	or	r0, r0, r0
     e4c:	30600001 	addik	r3, r0, 1
     e50:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     e54:	b810ffc0 	brid	-64		// e14
     e58:	10a00000 	addk	r5, r0, r0
     e5c:	e0a41a70 	lbui	r5, r4, 6768
     e60:	e8930004 	lwi	r4, r19, 4
     e64:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     e68:	c8642800 	lw	r3, r4, r5
     e6c:	64630008 	bsrli	r3, r3, 8
     e70:	b810ffa4 	brid	-92		// e14
     e74:	a4a30001 	andi	r5, r3, 1
     e78:	b9f40878 	brlid	r15, 2168	// 16f0 <Xil_Assert>
     e7c:	80000000 	or	r0, r0, r0
     e80:	30600001 	addik	r3, r0, 1
     e84:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     e88:	b810ff8c 	brid	-116		// e14
     e8c:	10b30000 	addk	r5, r19, r0

00000e90 <XTmrCtr_Reset>:
     e90:	11050000 	addk	r8, r5, r0
     e94:	3021ffe4 	addik	r1, r1, -28
     e98:	a48600ff 	andi	r4, r6, 255
     e9c:	f9e10000 	swi	r15, r1, 0
     ea0:	30a019cc 	addik	r5, r0, 6604
     ea4:	be08001c 	beqid	r8, 28		// ec0
     ea8:	30c00190 	addik	r6, r0, 400
     eac:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     eb0:	22400001 	addi	r18, r0, 1
     eb4:	16449003 	cmpu	r18, r4, r18
     eb8:	beb20024 	bgeid	r18, 36		// edc
     ebc:	30c00191 	addik	r6, r0, 401
     ec0:	b9f40830 	brlid	r15, 2096	// 16f0 <Xil_Assert>
     ec4:	80000000 	or	r0, r0, r0
     ec8:	e9e10000 	lwi	r15, r1, 0
     ecc:	30600001 	addik	r3, r0, 1
     ed0:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     ed4:	b60f0008 	rtsd	r15, 8
     ed8:	3021001c 	addik	r1, r1, 28
     edc:	e8680008 	lwi	r3, r8, 8
     ee0:	30a019cc 	addik	r5, r0, 6604
     ee4:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     ee8:	b0001111 	imm	4369
     eec:	aa431111 	xori	r18, r3, 4369
     ef0:	be32ffd0 	bneid	r18, -48		// ec0
     ef4:	30c00192 	addik	r6, r0, 402
     ef8:	e0c41a70 	lbui	r6, r4, 6768
     efc:	e8a80004 	lwi	r5, r8, 4
     f00:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     f04:	c8e53000 	lw	r7, r5, r6
     f08:	a0670020 	ori	r3, r7, 32
     f0c:	d8653000 	sw	r3, r5, r6
     f10:	e0841a70 	lbui	r4, r4, 6768
     f14:	e8680004 	lwi	r3, r8, 4
     f18:	d8e32000 	sw	r7, r3, r4
     f1c:	e9e10000 	lwi	r15, r1, 0
     f20:	b60f0008 	rtsd	r15, 8
     f24:	3021001c 	addik	r1, r1, 28

00000f28 <XTmrCtr_GetCaptureValue>:
     f28:	3021ffe0 	addik	r1, r1, -32
     f2c:	fa61001c 	swi	r19, r1, 28
     f30:	12650000 	addk	r19, r5, r0
     f34:	a48600ff 	andi	r4, r6, 255
     f38:	f9e10000 	swi	r15, r1, 0
     f3c:	30a019cc 	addik	r5, r0, 6604
     f40:	be130094 	beqid	r19, 148		// fd4
     f44:	30c00173 	addik	r6, r0, 371
     f48:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     f4c:	22400001 	addi	r18, r0, 1
     f50:	16449003 	cmpu	r18, r4, r18
     f54:	beb20030 	bgeid	r18, 48		// f84
     f58:	30c00174 	addik	r6, r0, 372
     f5c:	b9f40794 	brlid	r15, 1940	// 16f0 <Xil_Assert>
     f60:	80000000 	or	r0, r0, r0
     f64:	30600001 	addik	r3, r0, 1
     f68:	10a00000 	addk	r5, r0, r0
     f6c:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     f70:	10650000 	addk	r3, r5, r0
     f74:	e9e10000 	lwi	r15, r1, 0
     f78:	ea61001c 	lwi	r19, r1, 28
     f7c:	b60f0008 	rtsd	r15, 8
     f80:	30210020 	addik	r1, r1, 32
     f84:	e8730008 	lwi	r3, r19, 8
     f88:	30a019cc 	addik	r5, r0, 6604
     f8c:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     f90:	b0001111 	imm	4369
     f94:	aa431111 	xori	r18, r3, 4369
     f98:	be120020 	beqid	r18, 32		// fb8
     f9c:	30c00175 	addik	r6, r0, 373
     fa0:	b9f40750 	brlid	r15, 1872	// 16f0 <Xil_Assert>
     fa4:	80000000 	or	r0, r0, r0
     fa8:	30600001 	addik	r3, r0, 1
     fac:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     fb0:	b810ffc0 	brid	-64		// f70
     fb4:	10a00000 	addk	r5, r0, r0
     fb8:	e8730004 	lwi	r3, r19, 4
     fbc:	e0841a70 	lbui	r4, r4, 6768
     fc0:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
     fc4:	30630004 	addik	r3, r3, 4
     fc8:	c8a32000 	lw	r5, r3, r4
     fcc:	b810ffa8 	brid	-88		// f74
     fd0:	10650000 	addk	r3, r5, r0
     fd4:	b9f4071c 	brlid	r15, 1820	// 16f0 <Xil_Assert>
     fd8:	80000000 	or	r0, r0, r0
     fdc:	30600001 	addik	r3, r0, 1
     fe0:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
     fe4:	b810ff8c 	brid	-116		// f70
     fe8:	10b30000 	addk	r5, r19, r0

00000fec <XTmrCtr_SetResetValue>:
     fec:	10850000 	addk	r4, r5, r0
     ff0:	3021ffe4 	addik	r1, r1, -28
     ff4:	a50600ff 	andi	r8, r6, 255
     ff8:	f9e10000 	swi	r15, r1, 0
     ffc:	30a019cc 	addik	r5, r0, 6604
    1000:	be04001c 	beqid	r4, 28		// 101c
    1004:	30c00157 	addik	r6, r0, 343
    1008:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    100c:	22400001 	addi	r18, r0, 1
    1010:	16489003 	cmpu	r18, r8, r18
    1014:	beb20024 	bgeid	r18, 36		// 1038
    1018:	30c00158 	addik	r6, r0, 344
    101c:	b9f406d4 	brlid	r15, 1748	// 16f0 <Xil_Assert>
    1020:	80000000 	or	r0, r0, r0
    1024:	e9e10000 	lwi	r15, r1, 0
    1028:	30600001 	addik	r3, r0, 1
    102c:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    1030:	b60f0008 	rtsd	r15, 8
    1034:	3021001c 	addik	r1, r1, 28
    1038:	e8640008 	lwi	r3, r4, 8
    103c:	30a019cc 	addik	r5, r0, 6604
    1040:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    1044:	b0001111 	imm	4369
    1048:	aa431111 	xori	r18, r3, 4369
    104c:	be32ffd0 	bneid	r18, -48		// 101c
    1050:	30c00159 	addik	r6, r0, 345
    1054:	e8640004 	lwi	r3, r4, 4
    1058:	e0881a70 	lbui	r4, r8, 6768
    105c:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    1060:	30630004 	addik	r3, r3, 4
    1064:	d8e32000 	sw	r7, r3, r4
    1068:	e9e10000 	lwi	r15, r1, 0
    106c:	b60f0008 	rtsd	r15, 8
    1070:	3021001c 	addik	r1, r1, 28

00001074 <XTmrCtr_GetValue>:
    1074:	3021ffe0 	addik	r1, r1, -32
    1078:	fa61001c 	swi	r19, r1, 28
    107c:	12650000 	addk	r19, r5, r0
    1080:	a48600ff 	andi	r4, r6, 255
    1084:	f9e10000 	swi	r15, r1, 0
    1088:	30a019cc 	addik	r5, r0, 6604
    108c:	be130094 	beqid	r19, 148		// 1120
    1090:	30c00137 	addik	r6, r0, 311
    1094:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    1098:	22400001 	addi	r18, r0, 1
    109c:	16449003 	cmpu	r18, r4, r18
    10a0:	beb20030 	bgeid	r18, 48		// 10d0
    10a4:	30c00138 	addik	r6, r0, 312
    10a8:	b9f40648 	brlid	r15, 1608	// 16f0 <Xil_Assert>
    10ac:	80000000 	or	r0, r0, r0
    10b0:	30600001 	addik	r3, r0, 1
    10b4:	10a00000 	addk	r5, r0, r0
    10b8:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    10bc:	10650000 	addk	r3, r5, r0
    10c0:	e9e10000 	lwi	r15, r1, 0
    10c4:	ea61001c 	lwi	r19, r1, 28
    10c8:	b60f0008 	rtsd	r15, 8
    10cc:	30210020 	addik	r1, r1, 32
    10d0:	e8730008 	lwi	r3, r19, 8
    10d4:	30a019cc 	addik	r5, r0, 6604
    10d8:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    10dc:	b0001111 	imm	4369
    10e0:	aa431111 	xori	r18, r3, 4369
    10e4:	be120020 	beqid	r18, 32		// 1104
    10e8:	30c00139 	addik	r6, r0, 313
    10ec:	b9f40604 	brlid	r15, 1540	// 16f0 <Xil_Assert>
    10f0:	80000000 	or	r0, r0, r0
    10f4:	30600001 	addik	r3, r0, 1
    10f8:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    10fc:	b810ffc0 	brid	-64		// 10bc
    1100:	10a00000 	addk	r5, r0, r0
    1104:	e8730004 	lwi	r3, r19, 4
    1108:	e0841a70 	lbui	r4, r4, 6768
    110c:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    1110:	30630008 	addik	r3, r3, 8
    1114:	c8a32000 	lw	r5, r3, r4
    1118:	b810ffa8 	brid	-88		// 10c0
    111c:	10650000 	addk	r3, r5, r0
    1120:	b9f405d0 	brlid	r15, 1488	// 16f0 <Xil_Assert>
    1124:	80000000 	or	r0, r0, r0
    1128:	30600001 	addik	r3, r0, 1
    112c:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    1130:	b810ff8c 	brid	-116		// 10bc
    1134:	10b30000 	addk	r5, r19, r0

00001138 <XTmrCtr_Stop>:
    1138:	10850000 	addk	r4, r5, r0
    113c:	3021ffe4 	addik	r1, r1, -28
    1140:	a4e600ff 	andi	r7, r6, 255
    1144:	f9e10000 	swi	r15, r1, 0
    1148:	30a019cc 	addik	r5, r0, 6604
    114c:	be04001c 	beqid	r4, 28		// 1168
    1150:	30c0010d 	addik	r6, r0, 269
    1154:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    1158:	22400001 	addi	r18, r0, 1
    115c:	16479003 	cmpu	r18, r7, r18
    1160:	beb20024 	bgeid	r18, 36		// 1184
    1164:	30c0010e 	addik	r6, r0, 270
    1168:	b9f40588 	brlid	r15, 1416	// 16f0 <Xil_Assert>
    116c:	80000000 	or	r0, r0, r0
    1170:	e9e10000 	lwi	r15, r1, 0
    1174:	30600001 	addik	r3, r0, 1
    1178:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    117c:	b60f0008 	rtsd	r15, 8
    1180:	3021001c 	addik	r1, r1, 28
    1184:	e8640008 	lwi	r3, r4, 8
    1188:	30a019cc 	addik	r5, r0, 6604
    118c:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    1190:	b0001111 	imm	4369
    1194:	aa431111 	xori	r18, r3, 4369
    1198:	be32ffd0 	bneid	r18, -48		// 1168
    119c:	30c0010f 	addik	r6, r0, 271
    11a0:	e0a71a70 	lbui	r5, r7, 6768
    11a4:	e8840004 	lwi	r4, r4, 4
    11a8:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    11ac:	c8642800 	lw	r3, r4, r5
    11b0:	a463ff7f 	andi	r3, r3, -129
    11b4:	d8642800 	sw	r3, r4, r5
    11b8:	e9e10000 	lwi	r15, r1, 0
    11bc:	b60f0008 	rtsd	r15, 8
    11c0:	3021001c 	addik	r1, r1, 28

000011c4 <XTmrCtr_Start>:
    11c4:	11250000 	addk	r9, r5, r0
    11c8:	3021ffe4 	addik	r1, r1, -28
    11cc:	a50600ff 	andi	r8, r6, 255
    11d0:	f9e10000 	swi	r15, r1, 0
    11d4:	30a019cc 	addik	r5, r0, 6604
    11d8:	be09001c 	beqid	r9, 28		// 11f4
    11dc:	30c000d8 	addik	r6, r0, 216
    11e0:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    11e4:	22400001 	addi	r18, r0, 1
    11e8:	16489003 	cmpu	r18, r8, r18
    11ec:	beb20024 	bgeid	r18, 36		// 1210
    11f0:	30c000d9 	addik	r6, r0, 217
    11f4:	b9f404fc 	brlid	r15, 1276	// 16f0 <Xil_Assert>
    11f8:	80000000 	or	r0, r0, r0
    11fc:	e9e10000 	lwi	r15, r1, 0
    1200:	30600001 	addik	r3, r0, 1
    1204:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    1208:	b60f0008 	rtsd	r15, 8
    120c:	3021001c 	addik	r1, r1, 28
    1210:	e8690008 	lwi	r3, r9, 8
    1214:	30a019cc 	addik	r5, r0, 6604
    1218:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    121c:	b0001111 	imm	4369
    1220:	aa431111 	xori	r18, r3, 4369
    1224:	be32ffd0 	bneid	r18, -48		// 11f4
    1228:	30c000da 	addik	r6, r0, 218
    122c:	e0e81a70 	lbui	r7, r8, 6768
    1230:	e8a90004 	lwi	r5, r9, 4
    1234:	30600020 	addik	r3, r0, 32	// 20 <_vector_hw_exception>
    1238:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    123c:	c8853800 	lw	r4, r5, r7
    1240:	d8653800 	sw	r3, r5, r7
    1244:	e0c81a70 	lbui	r6, r8, 6768
    1248:	e8690004 	lwi	r3, r9, 4
    124c:	a0840080 	ori	r4, r4, 128
    1250:	d8833000 	sw	r4, r3, r6
    1254:	e9e10000 	lwi	r15, r1, 0
    1258:	b60f0008 	rtsd	r15, 8
    125c:	3021001c 	addik	r1, r1, 28

00001260 <XTmrCtr_Initialize>:
    1260:	3021ffe0 	addik	r1, r1, -32
    1264:	fa61001c 	swi	r19, r1, 28
    1268:	12650000 	addk	r19, r5, r0
    126c:	b0000000 	imm	0
    1270:	a466ffff 	andi	r3, r6, -1
    1274:	f9e10000 	swi	r15, r1, 0
    1278:	30a019cc 	addik	r5, r0, 6604
    127c:	be1300cc 	beqid	r19, 204		// 1348
    1280:	30c0006f 	addik	r6, r0, 111
    1284:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    1288:	b9f4fb18 	brlid	r15, -1256	// da0 <XTmrCtr_LookupConfig>
    128c:	10a30000 	addk	r5, r3, r0
    1290:	be0300a4 	beqid	r3, 164		// 1334
    1294:	31000002 	addik	r8, r0, 2
    1298:	e8a30004 	lwi	r5, r3, 4
    129c:	e0801a70 	lbui	r4, r0, 6768	// 1a70 <XTmrCtr_Offsets>
    12a0:	31000005 	addik	r8, r0, 5
    12a4:	c8642800 	lw	r3, r4, r5
    12a8:	a4630080 	andi	r3, r3, 128
    12ac:	be230088 	bneid	r3, 136		// 1334
    12b0:	30c50004 	addik	r6, r5, 4
    12b4:	e0601a71 	lbui	r3, r0, 6769
    12b8:	31200120 	addik	r9, r0, 288
    12bc:	c8832800 	lw	r4, r3, r5
    12c0:	a4e40080 	andi	r7, r4, 128
    12c4:	be270074 	bneid	r7, 116		// 1338
    12c8:	10680000 	addk	r3, r8, r0
    12cc:	f8b30004 	swi	r5, r19, 4
    12d0:	f8f30000 	swi	r7, r19, 0
    12d4:	f8f3000c 	swi	r7, r19, 12
    12d8:	f8f30010 	swi	r7, r19, 16
    12dc:	e0601a70 	lbui	r3, r0, 6768	// 1a70 <XTmrCtr_Offsets>
    12e0:	11070000 	addk	r8, r7, r0
    12e4:	d8e61800 	sw	r7, r6, r3
    12e8:	e0801a70 	lbui	r4, r0, 6768	// 1a70 <XTmrCtr_Offsets>
    12ec:	e8730004 	lwi	r3, r19, 4
    12f0:	d9232000 	sw	r9, r3, r4
    12f4:	e0a01a70 	lbui	r5, r0, 6768	// 1a70 <XTmrCtr_Offsets>
    12f8:	e8930004 	lwi	r4, r19, 4
    12fc:	d8e42800 	sw	r7, r4, r5
    1300:	e8730004 	lwi	r3, r19, 4
    1304:	e0801a71 	lbui	r4, r0, 6769
    1308:	30630004 	addik	r3, r3, 4
    130c:	d8e32000 	sw	r7, r3, r4
    1310:	e0a01a71 	lbui	r5, r0, 6769
    1314:	e8730004 	lwi	r3, r19, 4
    1318:	d9232800 	sw	r9, r3, r5
    131c:	e0c01a71 	lbui	r6, r0, 6769
    1320:	e8930004 	lwi	r4, r19, 4
    1324:	b0001111 	imm	4369
    1328:	30601111 	addik	r3, r0, 4369
    132c:	d8e43000 	sw	r7, r4, r6
    1330:	f8730008 	swi	r3, r19, 8
    1334:	10680000 	addk	r3, r8, r0
    1338:	e9e10000 	lwi	r15, r1, 0
    133c:	ea61001c 	lwi	r19, r1, 28
    1340:	b60f0008 	rtsd	r15, 8
    1344:	30210020 	addik	r1, r1, 32
    1348:	b9f403a8 	brlid	r15, 936	// 16f0 <Xil_Assert>
    134c:	80000000 	or	r0, r0, r0
    1350:	30600001 	addik	r3, r0, 1
    1354:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    1358:	b810ffdc 	brid	-36		// 1334
    135c:	11130000 	addk	r8, r19, r0

00001360 <XTmrCtr_InterruptHandler>:
    1360:	3021ffdc 	addik	r1, r1, -36
    1364:	fac10020 	swi	r22, r1, 32
    1368:	f9e10000 	swi	r15, r1, 0
    136c:	fa61001c 	swi	r19, r1, 28
    1370:	be0500bc 	beqid	r5, 188		// 142c
    1374:	12c50000 	addk	r22, r5, r0
    1378:	12600000 	addk	r19, r0, r0
    137c:	fa601bfc 	swi	r19, r0, 7164	// 1bfc <Xil_AssertStatus>
    1380:	e8960004 	lwi	r4, r22, 4
    1384:	e0b31a70 	lbui	r5, r19, 6768
    1388:	a4d300ff 	andi	r6, r19, 255
    138c:	c8642800 	lw	r3, r4, r5
    1390:	a4830100 	andi	r4, r3, 256
    1394:	a4630040 	andi	r3, r3, 64
    1398:	bc030008 	beqi	r3, 8		// 13a0
    139c:	bc240024 	bnei	r4, 36		// 13c0
    13a0:	32730001 	addik	r19, r19, 1
    13a4:	aa530002 	xori	r18, r19, 2
    13a8:	be32ffd8 	bneid	r18, -40		// 1380
    13ac:	e9e10000 	lwi	r15, r1, 0
    13b0:	ea61001c 	lwi	r19, r1, 28
    13b4:	eac10020 	lwi	r22, r1, 32
    13b8:	b60f0008 	rtsd	r15, 8
    13bc:	30210024 	addik	r1, r1, 36
    13c0:	e8760000 	lwi	r3, r22, 0
    13c4:	e896000c 	lwi	r4, r22, 12
    13c8:	e8b60010 	lwi	r5, r22, 16
    13cc:	30630001 	addik	r3, r3, 1
    13d0:	99fc2000 	brald	r15, r4
    13d4:	f8760000 	swi	r3, r22, 0
    13d8:	e0d31a70 	lbui	r6, r19, 6768
    13dc:	e8960004 	lwi	r4, r22, 4
    13e0:	c8a43000 	lw	r5, r4, r6
    13e4:	a4650011 	andi	r3, r5, 17
    13e8:	bc030014 	beqi	r3, 20		// 13fc
    13ec:	a0650100 	ori	r3, r5, 256
    13f0:	d8643000 	sw	r3, r4, r6
    13f4:	b810ffb0 	brid	-80		// 13a4
    13f8:	32730001 	addik	r19, r19, 1
    13fc:	a4a5ff7f 	andi	r5, r5, -129
    1400:	a0650020 	ori	r3, r5, 32
    1404:	d8643000 	sw	r3, r4, r6
    1408:	e0931a70 	lbui	r4, r19, 6768
    140c:	e8760004 	lwi	r3, r22, 4
    1410:	d8a32000 	sw	r5, r3, r4
    1414:	e0d31a70 	lbui	r6, r19, 6768
    1418:	e8960004 	lwi	r4, r22, 4
    141c:	a0650100 	ori	r3, r5, 256
    1420:	d8643000 	sw	r3, r4, r6
    1424:	b810ff80 	brid	-128		// 13a4
    1428:	32730001 	addik	r19, r19, 1
    142c:	30a019d8 	addik	r5, r0, 6616
    1430:	b9f402c0 	brlid	r15, 704	// 16f0 <Xil_Assert>
    1434:	30c0008b 	addik	r6, r0, 139
    1438:	30600001 	addik	r3, r0, 1
    143c:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    1440:	b810ff70 	brid	-144		// 13b0
    1444:	e9e10000 	lwi	r15, r1, 0

00001448 <XTmrCtr_SetHandler>:
    1448:	10850000 	addk	r4, r5, r0
    144c:	3021ffe4 	addik	r1, r1, -28
    1450:	11060000 	addk	r8, r6, r0
    1454:	f9e10000 	swi	r15, r1, 0
    1458:	30a019d8 	addik	r5, r0, 6616
    145c:	be040028 	beqid	r4, 40		// 1484
    1460:	30c0006c 	addik	r6, r0, 108	// 6c <_exit>
    1464:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    1468:	bc080050 	beqi	r8, 80		// 14b8
    146c:	e8640008 	lwi	r3, r4, 8
    1470:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    1474:	b0001111 	imm	4369
    1478:	aa431111 	xori	r18, r3, 4369
    147c:	be120024 	beqid	r18, 36		// 14a0
    1480:	30c0006e 	addik	r6, r0, 110
    1484:	b9f4026c 	brlid	r15, 620	// 16f0 <Xil_Assert>
    1488:	80000000 	or	r0, r0, r0
    148c:	30600001 	addik	r3, r0, 1
    1490:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    1494:	e9e10000 	lwi	r15, r1, 0
    1498:	b60f0008 	rtsd	r15, 8
    149c:	3021001c 	addik	r1, r1, 28
    14a0:	f8e40010 	swi	r7, r4, 16
    14a4:	f904000c 	swi	r8, r4, 12
    14a8:	e9e10000 	lwi	r15, r1, 0
    14ac:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    14b0:	b60f0008 	rtsd	r15, 8
    14b4:	3021001c 	addik	r1, r1, 28
    14b8:	b9f40238 	brlid	r15, 568	// 16f0 <Xil_Assert>
    14bc:	30c0006d 	addik	r6, r0, 109
    14c0:	30600001 	addik	r3, r0, 1
    14c4:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    14c8:	b810ffd0 	brid	-48		// 1498
    14cc:	e9e10000 	lwi	r15, r1, 0

000014d0 <XTmrCtr_GetOptions>:
    14d0:	3021ffe0 	addik	r1, r1, -32
    14d4:	fa61001c 	swi	r19, r1, 28
    14d8:	f9e10000 	swi	r15, r1, 0
    14dc:	12650000 	addk	r19, r5, r0
    14e0:	be0500c4 	beqid	r5, 196		// 15a4
    14e4:	a4c600ff 	andi	r6, r6, 255
    14e8:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    14ec:	22400001 	addi	r18, r0, 1
    14f0:	16469003 	cmpu	r18, r6, r18
    14f4:	bcb20030 	bgei	r18, 48		// 1524
    14f8:	30a019e8 	addik	r5, r0, 6632
    14fc:	b9f401f4 	brlid	r15, 500	// 16f0 <Xil_Assert>
    1500:	30c000c7 	addik	r6, r0, 199
    1504:	30600001 	addik	r3, r0, 1
    1508:	10c00000 	addk	r6, r0, r0
    150c:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    1510:	10660000 	addk	r3, r6, r0
    1514:	e9e10000 	lwi	r15, r1, 0
    1518:	ea61001c 	lwi	r19, r1, 28
    151c:	b60f0008 	rtsd	r15, 8
    1520:	30210020 	addik	r1, r1, 32
    1524:	e8650008 	lwi	r3, r5, 8
    1528:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    152c:	b0001111 	imm	4369
    1530:	aa431111 	xori	r18, r3, 4369
    1534:	bc120020 	beqi	r18, 32		// 1554
    1538:	30a019e8 	addik	r5, r0, 6632
    153c:	b9f401b4 	brlid	r15, 436	// 16f0 <Xil_Assert>
    1540:	30c000c8 	addik	r6, r0, 200
    1544:	30600001 	addik	r3, r0, 1
    1548:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    154c:	b810ffc4 	brid	-60		// 1510
    1550:	10c00000 	addk	r6, r0, r0
    1554:	e0861a70 	lbui	r4, r6, 6768
    1558:	e8650004 	lwi	r3, r5, 4
    155c:	10c00000 	addk	r6, r0, r0
    1560:	10e60000 	addk	r7, r6, r0
    1564:	c8a32000 	lw	r5, r3, r4
    1568:	f8c01bfc 	swi	r6, r0, 7164	// 1bfc <Xil_AssertStatus>
    156c:	b800001c 	bri	28		// 1588
    1570:	e86719fc 	lwi	r3, r7, 6652
    1574:	80c61800 	or	r6, r6, r3
    1578:	30e70008 	addik	r7, r7, 8
    157c:	aa470030 	xori	r18, r7, 48
    1580:	be12ff94 	beqid	r18, -108		// 1514
    1584:	10660000 	addk	r3, r6, r0
    1588:	e8671a00 	lwi	r3, r7, 6656
    158c:	84651800 	and	r3, r5, r3
    1590:	bc23ffe0 	bnei	r3, -32		// 1570
    1594:	e86719fc 	lwi	r3, r7, 6652
    1598:	a863ffff 	xori	r3, r3, -1
    159c:	b810ffdc 	brid	-36		// 1578
    15a0:	84c61800 	and	r6, r6, r3
    15a4:	30a019e8 	addik	r5, r0, 6632
    15a8:	b9f40148 	brlid	r15, 328	// 16f0 <Xil_Assert>
    15ac:	30c000c6 	addik	r6, r0, 198
    15b0:	30600001 	addik	r3, r0, 1
    15b4:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    15b8:	b810ff58 	brid	-168		// 1510
    15bc:	10d30000 	addk	r6, r19, r0

000015c0 <XTmrCtr_SetOptions>:
    15c0:	3021ffe4 	addik	r1, r1, -28
    15c4:	f9e10000 	swi	r15, r1, 0
    15c8:	be0500cc 	beqid	r5, 204		// 1694
    15cc:	a4c600ff 	andi	r6, r6, 255
    15d0:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    15d4:	22400001 	addi	r18, r0, 1
    15d8:	16469003 	cmpu	r18, r6, r18
    15dc:	bcb20024 	bgei	r18, 36		// 1600
    15e0:	30a019e8 	addik	r5, r0, 6632
    15e4:	b9f4010c 	brlid	r15, 268	// 16f0 <Xil_Assert>
    15e8:	30c0008c 	addik	r6, r0, 140
    15ec:	30600001 	addik	r3, r0, 1
    15f0:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    15f4:	e9e10000 	lwi	r15, r1, 0
    15f8:	b60f0008 	rtsd	r15, 8
    15fc:	3021001c 	addik	r1, r1, 28
    1600:	e8650008 	lwi	r3, r5, 8
    1604:	f8001bfc 	swi	r0, r0, 7164	// 1bfc <Xil_AssertStatus>
    1608:	b0001111 	imm	4369
    160c:	aa431111 	xori	r18, r3, 4369
    1610:	be120028 	beqid	r18, 40		// 1638
    1614:	11000000 	addk	r8, r0, r0
    1618:	30a019e8 	addik	r5, r0, 6632
    161c:	b9f400d4 	brlid	r15, 212	// 16f0 <Xil_Assert>
    1620:	30c0008d 	addik	r6, r0, 141
    1624:	e9e10000 	lwi	r15, r1, 0
    1628:	30600001 	addik	r3, r0, 1
    162c:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    1630:	b60f0008 	rtsd	r15, 8
    1634:	3021001c 	addik	r1, r1, 28
    1638:	f9001bfc 	swi	r8, r0, 7164	// 1bfc <Xil_AssertStatus>
    163c:	b810001c 	brid	28		// 1658
    1640:	10880000 	addk	r4, r8, r0
    1644:	e8641a00 	lwi	r3, r4, 6656
    1648:	30840008 	addik	r4, r4, 8
    164c:	aa440030 	xori	r18, r4, 48
    1650:	be12002c 	beqid	r18, 44		// 167c
    1654:	81081800 	or	r8, r8, r3
    1658:	e86419fc 	lwi	r3, r4, 6652
    165c:	84671800 	and	r3, r7, r3
    1660:	bc23ffe4 	bnei	r3, -28		// 1644
    1664:	e8641a00 	lwi	r3, r4, 6656
    1668:	30840008 	addik	r4, r4, 8
    166c:	a863ffff 	xori	r3, r3, -1
    1670:	aa440030 	xori	r18, r4, 48
    1674:	be32ffe4 	bneid	r18, -28		// 1658
    1678:	85081800 	and	r8, r8, r3
    167c:	e0861a70 	lbui	r4, r6, 6768
    1680:	e8650004 	lwi	r3, r5, 4
    1684:	d9032000 	sw	r8, r3, r4
    1688:	e9e10000 	lwi	r15, r1, 0
    168c:	b60f0008 	rtsd	r15, 8
    1690:	3021001c 	addik	r1, r1, 28
    1694:	30a019e8 	addik	r5, r0, 6632
    1698:	b9f40058 	brlid	r15, 88	// 16f0 <Xil_Assert>
    169c:	30c0008b 	addik	r6, r0, 139
    16a0:	30600001 	addik	r3, r0, 1
    16a4:	f8601bfc 	swi	r3, r0, 7164	// 1bfc <Xil_AssertStatus>
    16a8:	b810ff50 	brid	-176		// 15f8
    16ac:	e9e10000 	lwi	r15, r1, 0

000016b0 <XAssert>:
    16b0:	e8601bdc 	lwi	r3, r0, 7132	// 1bdc <XAssertCallbackRoutine>
    16b4:	3021ffe4 	addik	r1, r1, -28
    16b8:	be030010 	beqid	r3, 16		// 16c8
    16bc:	f9e10000 	swi	r15, r1, 0
    16c0:	99fc1800 	brald	r15, r3
    16c4:	80000000 	or	r0, r0, r0
    16c8:	e8601a74 	lwi	r3, r0, 6772	// 1a74 <XWaitInAssert>
    16cc:	be230000 	bneid	r3, 0		// 16cc
    16d0:	e9e10000 	lwi	r15, r1, 0
    16d4:	b60f0008 	rtsd	r15, 8
    16d8:	3021001c 	addik	r1, r1, 28

000016dc <XAssertSetCallback>:
    16dc:	f8a01bdc 	swi	r5, r0, 7132	// 1bdc <XAssertCallbackRoutine>
    16e0:	b60f0008 	rtsd	r15, 8
    16e4:	80000000 	or	r0, r0, r0

000016e8 <XNullHandler>:
    16e8:	b60f0008 	rtsd	r15, 8
    16ec:	80000000 	or	r0, r0, r0

000016f0 <Xil_Assert>:
    16f0:	e8601be0 	lwi	r3, r0, 7136	// 1be0 <Xil_AssertCallbackRoutine>
    16f4:	3021ffe4 	addik	r1, r1, -28
    16f8:	be030010 	beqid	r3, 16		// 1708
    16fc:	f9e10000 	swi	r15, r1, 0
    1700:	99fc1800 	brald	r15, r3
    1704:	80000000 	or	r0, r0, r0
    1708:	e8601a78 	lwi	r3, r0, 6776	// 1a78 <Xil_AssertWait>
    170c:	be230000 	bneid	r3, 0		// 170c
    1710:	e9e10000 	lwi	r15, r1, 0
    1714:	b60f0008 	rtsd	r15, 8
    1718:	3021001c 	addik	r1, r1, 28

0000171c <Xil_AssertSetCallback>:
    171c:	f8a01be0 	swi	r5, r0, 7136	// 1be0 <Xil_AssertCallbackRoutine>
    1720:	b60f0008 	rtsd	r15, 8
    1724:	80000000 	or	r0, r0, r0

00001728 <exit>:
    1728:	3021ffe0 	addik	r1, r1, -32
    172c:	10c00000 	addk	r6, r0, r0
    1730:	fa61001c 	swi	r19, r1, 28
    1734:	f9e10000 	swi	r15, r1, 0
    1738:	b9f40024 	brlid	r15, 36	// 175c <__call_exitprocs>
    173c:	12650000 	addk	r19, r5, r0
    1740:	e8a01a2c 	lwi	r5, r0, 6700	// 1a2c <_global_impure_ptr>
    1744:	e8650028 	lwi	r3, r5, 40
    1748:	bc03000c 	beqi	r3, 12		// 1754
    174c:	99fc1800 	brald	r15, r3
    1750:	80000000 	or	r0, r0, r0
    1754:	b9f4e918 	brlid	r15, -5864	// 6c <_exit>
    1758:	10b30000 	addk	r5, r19, r0

0000175c <__call_exitprocs>:
    175c:	e8601a2c 	lwi	r3, r0, 6700	// 1a2c <_global_impure_ptr>
    1760:	3021ffd0 	addik	r1, r1, -48
    1764:	fb010028 	swi	r24, r1, 40
    1768:	fb21002c 	swi	r25, r1, 44
    176c:	f9e10000 	swi	r15, r1, 0
    1770:	fa61001c 	swi	r19, r1, 28
    1774:	fac10020 	swi	r22, r1, 32
    1778:	fae10024 	swi	r23, r1, 36
    177c:	eac30048 	lwi	r22, r3, 72
    1780:	13250000 	addk	r25, r5, r0
    1784:	be160044 	beqid	r22, 68		// 17c8
    1788:	13060000 	addk	r24, r6, r0
    178c:	e8760004 	lwi	r3, r22, 4
    1790:	3263ffff 	addik	r19, r3, -1
    1794:	be530034 	bltid	r19, 52		// 17c8
    1798:	eaf60088 	lwi	r23, r22, 136
    179c:	bc060064 	beqi	r6, 100		// 1800
    17a0:	bc1700d0 	beqi	r23, 208		// 1870
    17a4:	64b30402 	bslli	r5, r19, 2
    17a8:	1065b800 	addk	r3, r5, r23
    17ac:	e8830080 	lwi	r4, r3, 128
    17b0:	1644c000 	rsubk	r18, r4, r24
    17b4:	be120104 	beqid	r18, 260		// 18b8
    17b8:	1065b000 	addk	r3, r5, r22
    17bc:	3273ffff 	addik	r19, r19, -1
    17c0:	aa53ffff 	xori	r18, r19, -1
    17c4:	bc32ffe0 	bnei	r18, -32		// 17a4
    17c8:	e9e10000 	lwi	r15, r1, 0
    17cc:	ea61001c 	lwi	r19, r1, 28
    17d0:	eac10020 	lwi	r22, r1, 32
    17d4:	eae10024 	lwi	r23, r1, 36
    17d8:	eb010028 	lwi	r24, r1, 40
    17dc:	eb21002c 	lwi	r25, r1, 44
    17e0:	b60f0008 	rtsd	r15, 8
    17e4:	30210030 	addik	r1, r1, 48
    17e8:	99fc3800 	brald	r15, r7
    17ec:	c8b72800 	lw	r5, r23, r5
    17f0:	3273ffff 	addik	r19, r19, -1
    17f4:	aa53ffff 	xori	r18, r19, -1
    17f8:	be12ffd4 	beqid	r18, -44		// 17cc
    17fc:	e9e10000 	lwi	r15, r1, 0
    1800:	e8960004 	lwi	r4, r22, 4
    1804:	64b30402 	bslli	r5, r19, 2
    1808:	3084ffff 	addik	r4, r4, -1
    180c:	1065b000 	addk	r3, r5, r22
    1810:	16449800 	rsubk	r18, r4, r19
    1814:	be12009c 	beqid	r18, 156		// 18b0
    1818:	e8e30008 	lwi	r7, r3, 8
    181c:	1065b000 	addk	r3, r5, r22
    1820:	f8030008 	swi	r0, r3, 8
    1824:	bc07ffcc 	beqi	r7, -52		// 17f0
    1828:	be17006c 	beqid	r23, 108		// 1894
    182c:	30800001 	addik	r4, r0, 1
    1830:	e8770100 	lwi	r3, r23, 256
    1834:	44849c00 	bsll	r4, r4, r19
    1838:	84641800 	and	r3, r4, r3
    183c:	bc030058 	beqi	r3, 88		// 1894
    1840:	e8770104 	lwi	r3, r23, 260
    1844:	84641800 	and	r3, r4, r3
    1848:	bc23ffa0 	bnei	r3, -96		// 17e8
    184c:	c8d72800 	lw	r6, r23, r5
    1850:	10b90000 	addk	r5, r25, r0
    1854:	99fc3800 	brald	r15, r7
    1858:	3273ffff 	addik	r19, r19, -1
    185c:	aa53ffff 	xori	r18, r19, -1
    1860:	be32ffa0 	bneid	r18, -96		// 1800
    1864:	e9e10000 	lwi	r15, r1, 0
    1868:	b810ff68 	brid	-152		// 17d0
    186c:	ea61001c 	lwi	r19, r1, 28
    1870:	3273ffff 	addik	r19, r19, -1
    1874:	aa53ffff 	xori	r18, r19, -1
    1878:	be12ff50 	beqid	r18, -176		// 17c8
    187c:	3273ffff 	addik	r19, r19, -1
    1880:	aa53ffff 	xori	r18, r19, -1
    1884:	be32fff0 	bneid	r18, -16		// 1874
    1888:	3273ffff 	addik	r19, r19, -1
    188c:	b810ff40 	brid	-192		// 17cc
    1890:	e9e10000 	lwi	r15, r1, 0
    1894:	99fc3800 	brald	r15, r7
    1898:	3273ffff 	addik	r19, r19, -1
    189c:	aa53ffff 	xori	r18, r19, -1
    18a0:	be32ff60 	bneid	r18, -160		// 1800
    18a4:	e9e10000 	lwi	r15, r1, 0
    18a8:	b810ff28 	brid	-216		// 17d0
    18ac:	ea61001c 	lwi	r19, r1, 28
    18b0:	b810ff74 	brid	-140		// 1824
    18b4:	fa760004 	swi	r19, r22, 4
    18b8:	e8960004 	lwi	r4, r22, 4
    18bc:	3084ffff 	addik	r4, r4, -1
    18c0:	16449800 	rsubk	r18, r4, r19
    18c4:	be120058 	beqid	r18, 88		// 191c
    18c8:	e8e30008 	lwi	r7, r3, 8
    18cc:	1065b000 	addk	r3, r5, r22
    18d0:	f8030008 	swi	r0, r3, 8
    18d4:	bc07fee8 	beqi	r7, -280		// 17bc
    18d8:	be170038 	beqid	r23, 56		// 1910
    18dc:	30800001 	addik	r4, r0, 1
    18e0:	e8770100 	lwi	r3, r23, 256
    18e4:	44849c00 	bsll	r4, r4, r19
    18e8:	84641800 	and	r3, r4, r3
    18ec:	bc030024 	beqi	r3, 36		// 1910
    18f0:	e8770104 	lwi	r3, r23, 260
    18f4:	84641800 	and	r3, r4, r3
    18f8:	bc23002c 	bnei	r3, 44		// 1924
    18fc:	c8d72800 	lw	r6, r23, r5
    1900:	99fc3800 	brald	r15, r7
    1904:	10b90000 	addk	r5, r25, r0
    1908:	b810feb8 	brid	-328		// 17c0
    190c:	3273ffff 	addik	r19, r19, -1
    1910:	99fc3800 	brald	r15, r7
    1914:	3273ffff 	addik	r19, r19, -1
    1918:	b800fea8 	bri	-344		// 17c0
    191c:	b810ffb8 	brid	-72		// 18d4
    1920:	fa760004 	swi	r19, r22, 4
    1924:	99fc3800 	brald	r15, r7
    1928:	c8b72800 	lw	r5, r23, r5
    192c:	b810fe94 	brid	-364		// 17c0
    1930:	3273ffff 	addik	r19, r19, -1

00001934 <__do_global_ctors_aux>:
    1934:	e8601b74 	lwi	r3, r0, 7028	// 1b74 <__data_end>
    1938:	3021ffe0 	addik	r1, r1, -32
    193c:	fa61001c 	swi	r19, r1, 28
    1940:	32601b74 	addik	r19, r0, 7028	// 1b74 <__data_end>
    1944:	aa43ffff 	xori	r18, r3, -1
    1948:	be12001c 	beqid	r18, 28		// 1964
    194c:	f9e10000 	swi	r15, r1, 0
    1950:	99fc1800 	brald	r15, r3
    1954:	3273fffc 	addik	r19, r19, -4
    1958:	e8730000 	lwi	r3, r19, 0
    195c:	aa43ffff 	xori	r18, r3, -1
    1960:	bc32fff0 	bnei	r18, -16		// 1950
    1964:	e9e10000 	lwi	r15, r1, 0
    1968:	ea61001c 	lwi	r19, r1, 28
    196c:	b60f0008 	rtsd	r15, 8
    1970:	30210020 	addik	r1, r1, 32
Disassembly of section .init:

00001974 <__init>:
    1974:	3021fff8 	addik	r1, r1, -8
    1978:	d9e00800 	sw	r15, r0, r1
    197c:	316023f0 	addik	r11, r0, 9200
    1980:	940bc802 	mts	rshr, r11
    1984:	31602004 	addik	r11, r0, 8196
    1988:	940bc800 	mts	rslr, r11
    198c:	b9f4e744 	brlid	r15, -6332	// d0 <frame_dummy>
    1990:	80000000 	or	r0, r0, r0
    1994:	b9f4ffa0 	brlid	r15, -96	// 1934 <__do_global_ctors_aux>
    1998:	80000000 	or	r0, r0, r0
    199c:	c9e00800 	lw	r15, r0, r1
    19a0:	b60f0008 	rtsd	r15, 8
    19a4:	30210008 	addik	r1, r1, 8
Disassembly of section .fini:

000019a8 <__fini>:
    19a8:	3021fff8 	addik	r1, r1, -8
    19ac:	d9e00800 	sw	r15, r0, r1
    19b0:	b9f4e6c0 	brlid	r15, -6464	// 70 <__do_global_dtors_aux>
    19b4:	80000000 	or	r0, r0, r0
    19b8:	c9e00800 	lw	r15, r0, r1
    19bc:	b60f0008 	rtsd	r15, 8
    19c0:	30210008 	addik	r1, r1, 8
