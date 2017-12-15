/*
** Copyright (c) 2017 Markku Pulkkinen. All rights reserved.
** Contact: markku.j.pulkkinen@gmail.com
**
** This file is part of libwarputil software distribution.
**
** This software is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This software is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>

#include "scrypt.h"
#include "sha256.h"

#include "WarpKeyGenerator.h"

using namespace cppcrypto;

/* Warp crypto key generation algorithm
 * ***************************************************************************
 * s1 = scrypt.hash(password=phrase+'\x01', salt=saltPhrase+'\x01',
 *                  N=1<<18, r=8, p=1, buflen=32)
 * s2 = pbkdf2(phrase+'\x02', saltPhrase+'\x02',
 *             1<<16, keylen=32, prf='hmac-sha256')
 * key = ''.join(chr(ord(a) ^ ord(b)) for a,b in zip(s1,s2))
 * key_hex = hexlify(key)
 */

int WarpKeyGenerator::generate(const ByteVect& pwd, const ByteVect& salt,
                               SecretKey& out) {
  // sanity checks
  if (pwd.size() < 2)
    throw std::invalid_argument("WarpKeyGenerator::password too short");

  uint8_t s1[32];
  uint8_t s2[32];

  /// \todo: run algorithms in separate threads
  {
    // do scrypt
    std::vector<uint8_t> a(pwd);
    a.push_back('\01');
    std::vector<uint8_t> b(salt);
    b.push_back('\01');

    hmac h(sha256(), a.data(), a.size());
    scrypt(h, b.data(), b.size(), (1 << 18), 8, 1, s1, sizeof(s1));
  }
  {
    // do pbkdf2
    std::vector<uint8_t> a(pwd);
    a.push_back('\02');
    std::vector<uint8_t> b(salt);
    b.push_back('\02');

    hmac h(sha256(), a.data(), a.size());
    pbkdf2(h, b.data(), b.size(), (1 << 16), s2, sizeof(s2));
  }

  // do XOR using s1 and s2 and save results to out buf
  uint8_t key[32];
  std::transform(std::begin(s1), std::end(s1), std::begin(s2), std::begin(key),
                 std::bit_xor<uint8_t>());

  std::copy(std::begin(key), std::end(key), std::begin(out));
  return 0;
}
