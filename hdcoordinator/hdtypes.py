# Copyright (c) 2012, Cornell University
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of HyperDex nor the names of its contributors may be
#       used to endorse or promote products derived from this software without
#       specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


import collections


class Dimension(object):

    def __init__(self, name, datatype):
        self._name = name
        self._datatype = datatype

    @property
    def name(self):
        return self._name

    @property
    def datatype(self):
        return self._datatype


class Space(object):

    def __init__(self, name, dimensions, subspaces):
        self._name = name
        self._dimensions = tuple(dimensions)
        self._subspaces = tuple(subspaces)

    @property
    def name(self):
        return self._name

    @property
    def dimensions(self):
        return self._dimensions

    @property
    def subspaces(self):
        return self._subspaces


class Subspace(object):

    def __init__(self, dimensions, nosearch, regions):
        self._dimensions = tuple(dimensions)
        self._nosearch = tuple(nosearch)
        self._regions = tuple(regions)

    @property
    def dimensions(self):
        return self._dimensions

    @property
    def nosearch(self):
        return self._nosearch

    @property
    def regions(self):
        return self._regions


class Region(object):

    def __init__(self, prefix, mask, desired_f):
        self._prefix = prefix
        self._mask = mask
        self._desired_f = desired_f
        self._replicas = []

    @property
    def prefix(self):
        return self._prefix

    @property
    def mask(self):
        return self._mask

    @property
    def desired_f(self):
        return self._desired_f

    @property
    def current_f(self):
        return len(self._replicas) - 1

    @property
    def replicas(self):
        return tuple(self._replicas)

    def add_replica(self, replica):
        assert replica is not None
        self._replicas.append(replica)

    def remove_replicas(self, badreplicas):
        newreplicas = []
        for replica in self._replicas:
            if replica not in badreplicas:
                newreplicas.append(replica)
        self._replicas = newreplicas

    def __eq__(self, other):
        return self.prefix == other.prefix and \
               self.mask == other.mask and \
               self.desired_f == other.desired_f

    def __ne__(self, other):
        return not (self == other)

    def __repr__(self):
        return 'Region(prefix={0}, mask={1}, desired_f={2})' \
               .format(self.prefix, hex(self.mask), self.desired_f)


InstanceBindings = collections.namedtuple('Instance', 'addr inport inver outport outver')


class Instance(object):

    def __init__(self, addr, inport, inver, outport, outver, pid, token):
        self._addr = addr
        self._inport = inport
        self._inver = inver
        self._outport = outport
        self._outver = outver
        self._pid = pid
        self._token = token
        self._configs = []
        self._last_acked = 0
        self._last_rejected = 0

    @property
    def addr(self):
        return self._addr

    @property
    def inport(self):
        return self._inport

    @property
    def inver(self):
        return self._inver

    @property
    def outport(self):
        return self._outport

    @property
    def outver(self):
        return self._outver

    def add_config(self, num, data):
        assert not self._configs or num > self._configs[-1][0]
        self._configs.append((num, data))

    def ack_config(self, num):
        if not self._configs:
            raise RuntimeError("acking config when none are present")
        if self._configs[0][0] != num:
            raise RuntimeError("acking config number which does not match the oldest pending")
        self._configs = self._configs[1:]
        self._last_acked = num

    def reject_config(self, num):
        if not self._configs:
            raise RuntimeError("rejecting config when none are present")
        if self._configs[0][0] != num:
            raise RuntimeError("rejecting config number which does not match the oldest pending")
        self._configs = self._configs[1:]
        self._last_rejected = num

    def next_config(self):
        if self._configs:
            return self._configs[0]
        return (None, None)

    def bindings(self):
        return InstanceBindings(self._addr, self._inport, self._inver,
                                self._outport, self._outver)
