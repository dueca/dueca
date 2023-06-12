import msgpack

# normal, fully filled
d1 = dict(gaze_normals_3d={"0":[0, 0, 0], "1": [1, 1, 1]},
          eye_centers_3d={"0":[0, 0, 0], "1": [1, 1, 1]})

# normal, fully filled with floats
d6 = dict(gaze_normals_3d={"0":[0.1, 0, 0.01], "1": [1, 1, 1]},
          eye_centers_3d={"0":[0, 0.3, 0], "1": [1, 1.1, 1]})

# no entries in the dict
d2 = dict()

# just one entry
d3 = dict(eye_centers_3d={"0":[0, 0, 0], "1": [1, 1, 1]})

# arrays as None/nil
d4 = dict(gaze_normals_3d={"0": None, "1": [1, 1, 1]},
          eye_centers_3d={"0":[0, 0, 0], "1": None})

# entries are None/nil
d5 = dict(gaze_normals_3d=None, eye_centers_3d=None)

with open("PupilRemoteGaze2.msgpack", mode='wb') as f:
    for d in d1, d2, d3, d4, d5, d6:
        f.write(msgpack.packb(d))

with open("PupilRemoteGaze2.msgpack", mode='rb') as f:
    up = msgpack.Unpacker(f, raw=False)
    for d in up:
        print(d)
