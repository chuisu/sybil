import tensorflow as tf

# for each file, call spleeter

# call loader on directory

# call

def custom_loss(y_true, y_pred):
    in_range_mask = tf.logical_and(y_pred >= 130, y_pred <= 1046.5)
    out_of_range_mask = tf.logical_not(in_range_mask)

    zero_mask = tf.equal(y_true, 0.0)
    non_zero_mask = tf.logical_not(zero_mask)

    zero_pred_mask = tf.equal(y_pred, 0.0)
    non_zero_pred_mask = tf.logical_not(zero_pred_mask)

    in_range_error = tf.square(y_true - y_pred) * tf.cast(in_range_mask, dtype=tf.float32)
    out_of_range_error = tf.square(y_true - y_pred) * tf.cast(out_of_range_mask, dtype=tf.float32)

    zero_error = tf.square(y_true - y_pred) * tf.cast(tf.logical_and(zero_mask, non_zero_pred_mask), dtype=tf.float32)
    non_zero_error = tf.square(y_true - y_pred) * tf.cast(tf.logical_and(non_zero_mask, zero_pred_mask), dtype=tf.float32)

    out_of_range_penalty = 1  # increase this value to apply a larger penalty for out-of-range predictions
    zero_penalty = 0.001  # increase this value to apply a larger penalty for non-zero predictions when true value is zero
    non_zero_penalty = 0.1  # increase this value to apply a larger penalty for zero predictions when true value is non-zero

    out_of_range_error *= out_of_range_penalty
    zero_error *= zero_penalty
    non_zero_error *= non_zero_penalty

    return tf.reduce_mean(in_range_error + out_of_range_error + zero_error + non_zero_error)