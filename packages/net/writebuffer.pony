use "collections"

type _ChunkNodeT is ListNode[(Array[U8] ref, USize)] ref

class WriteBuffer
  let _chunks: List[(Array[U8] ref, USize)] = _chunks.create()
  var _length: USize = 0
  let _alloc_size: USize
  var _current_array: Array[U8] = Array[U8]
  var _current_array_idx: USize = 0

  new create(alloc_size: USize = 1024) =>
    _alloc_size = alloc_size
    _alloc()

  fun ref get_write_buffer_point(): WriteBufferPoint^ ? =>
    WriteBufferPoint(_chunks.tail(), _current_array_idx, this)

  fun ref _alloc() =>
    _current_array = Array[U8].undefined(_alloc_size)
    _current_array_idx = 0
    _chunks.push((_current_array, _current_array_idx))

  fun size(): USize =>
    _length

  fun ref clear(): WriteBuffer^ =>
    _chunks.clear()
    _length = 0
    this

  fun ref to_array(): Array[U8] ref =>
    let length: USize = _length
    var bytes = Array[U8].undefined(length)

    var dst_offset = USize(0)

    for (data, len) in _chunks.values() do
      data.copy_to(bytes, 0, dst_offset, len)
      dst_offset = dst_offset + len
    end
    bytes

  fun ref _byte(b: U8, node: _ChunkNodeT, idx: USize): (_ChunkNodeT, USize) ? =>
    (_, let array_used) = node()
    if (node is _chunks.tail()) and
        ((idx == array_used) or (idx == _alloc_size)) then
      return _byte_at_end(b)
    end

    (let n: _ChunkNodeT, let i: USize)  = (
      if (idx == _alloc_size) then
        (node.next() as _ChunkNodeT, 0)
      else
        (node, idx)
      end)

    (let a, _) = n()
    a(i) = b
    (n, i + 1)

  fun ref _byte_at_end(b: U8): (_ChunkNodeT, USize) ? =>
    if _current_array_idx == _alloc_size then
      _alloc()
    end

    _current_array(_current_array_idx) = b
    _current_array_idx = _current_array_idx + 1
    _chunks.tail().update((_current_array, _current_array_idx))
    _length = _length + 1
    (_chunks.tail(), _current_array_idx)
  
  fun ref byte(b: U8, node: _ChunkNodeT, idx: USize):
    (_ChunkNodeT, USize) ?
  =>
    _byte(b, node, idx)

interface IWriteBufferPoint
  fun ref _byte(b: U8): IWriteBufferPoint^ ?
  fun ref u8(value: U8): IWriteBufferPoint^ ?
  fun ref i8(value: I8): IWriteBufferPoint^ ?
  fun ref u16_be(value: U16): IWriteBufferPoint^ ?
  fun ref i16_be(value: I16): IWriteBufferPoint^ ?
  fun ref u32_be(value: U32): IWriteBufferPoint^ ?
  fun ref i32_be(value: I32): IWriteBufferPoint^ ?
  fun ref u64_be(value: U64): IWriteBufferPoint^ ?
  fun ref i64_be(value: I64): IWriteBufferPoint^ ?
  fun ref u16_le(value: U16): IWriteBufferPoint^ ?
  fun ref i16_le(value: I16): IWriteBufferPoint^ ?
  fun ref u32_le(value: U32): IWriteBufferPoint^ ?
  fun ref i32_le(value: I32): IWriteBufferPoint^ ?
  fun ref u64_le(value: U64): IWriteBufferPoint^ ?
  fun ref i64_le(value: I64): IWriteBufferPoint^ ?

class WriteBufferPoint
  var _node: ListNode[(Array[U8] ref, USize)]
  var _idx: USize
  let _write_buffer: WriteBuffer

  new create(node: ListNode[(Array[U8] ref, USize)], idx: USize,
    write_buffer: WriteBuffer)
  =>
    _node = node
    _idx = idx
    _write_buffer = write_buffer

  fun ref _byte(b: U8): IWriteBufferPoint^ ? =>
    (_node, _idx) = _write_buffer.byte(b, _node, _idx)
    this

  fun ref u8(value: U8): IWriteBufferPoint^ ? =>
    _byte(value)

  fun ref i8(value: I8): IWriteBufferPoint^ ? =>
    _byte(value.u8())

  fun ref u16_be(value: U16): IWriteBufferPoint^ ? =>
    _byte((value>>8).u8())._byte(value.u8())

  fun ref i16_be(value: I16): IWriteBufferPoint^ ? =>
    u16_be(value.u16())

  fun ref u32_be(value: U32): IWriteBufferPoint^ ? =>
    u16_be((value>>16).u16()).u16_be(value.u16())

  fun ref i32_be(value: I32): IWriteBufferPoint^ ? =>
    u32_be(value.u32())

  fun ref u64_be(value: U64): IWriteBufferPoint^ ? =>
    u32_be((value>>32).u32()).u32_be(value.u32())

  fun ref i64_be(value: I64): IWriteBufferPoint^ ? =>
    u64_be(value.u64())

  fun ref u16_le(value: U16): IWriteBufferPoint^ ? =>
    _byte(value.u8())._byte((value>>8).u8())

  fun ref i16_le(value: I16): IWriteBufferPoint^ ? =>
    u16_le(value.u16())

  fun ref u32_le(value: U32): IWriteBufferPoint^ ? =>
    u16_le(value.u16()).u16_le((value>>16).u16())

  fun ref i32_le(value: I32): IWriteBufferPoint^ ? =>
    u32_le(value.u32())

  fun ref u64_le(value: U64): IWriteBufferPoint^ ? =>
    u32_le(value.u32()).u32_le((value>>32).u32())

  fun ref i64_le(value: I64): IWriteBufferPoint^ ? =>
    u64_le(value.u64())