use "collections"

class WriteBuffer
  let _chunks: List[(Array[U8] ref, USize)] = _chunks.create()
  var _length: USize = 0
  let _alloc_size: USize
  var _current_array: Array[U8] = Array[U8]
  var _current_array_idx: USize = 0

  new create(alloc_size: USize = 1024) =>
    _alloc_size = alloc_size
    _alloc()

  fun ref get_write_point(): WriteBufferPoint^ ? =>
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

  fun ref _byte(b: U8, loc: ((ListNode[(Array[U8] ref, USize)] ref, USize) | None)):
    (ListNode[(Array[U8] ref, USize)] ref, USize) ?
  =>
    match loc
    | (let node: ListNode[(Array[U8] ref, USize)] ref, let idx: USize) =>
      let n: ListNode[(Array[U8] ref, USize)] ref  = (if idx == _alloc_size then
        if node.next() is None then
          return _byte(b, None)
        end
        node.next() as ListNode[(Array[U8] ref, USize)] ref
      else
        node as ListNode[(Array[U8] ref, USize)] ref
      end)
      (let a, _) = n()
      a(idx) = b
      (n, idx + 1)
    else
      if _current_array_idx == _alloc_size then
       _alloc()
      end

      _current_array(_current_array_idx) = b
      _current_array_idx = _current_array_idx + 1
      _chunks.tail().update((_current_array, _current_array_idx))
      _length = _length + 1
      (_chunks.tail(), _current_array_idx)
    end
  
  fun ref byte(b: U8, loc: ((ListNode[(Array[U8] ref, USize)] ref, USize) | None) = None):
    (ListNode[(Array[U8] ref, USize)] ref, USize) ?
  =>
    _byte(b, loc)

interface IWriteBufferPoint
  fun ref byte(b: U8): IWriteBufferPoint^ ?

class WriteBufferPoint is IWriteBufferPoint
  var _node: ListNode[(Array[U8] ref, USize)]
  var _idx: USize
  let _write_buffer: WriteBuffer

  new create(node: ListNode[(Array[U8] ref, USize)], idx: USize,
    write_buffer: WriteBuffer)
  =>
    _node = node
    _idx = idx
    _write_buffer = write_buffer

  fun ref byte(b: U8): IWriteBufferPoint^ ? =>
    (_node, _idx) = _write_buffer.byte(b, (_node, _idx))
    this