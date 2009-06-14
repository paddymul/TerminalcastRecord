

(defun follow-point () 
  " this function sets up a hook to follow the point in another window "
  (add-hook 'post-command-hook
            '(lambda ()
               (set-window-point 
                (car (remove-if-not 
                     '(lambda (test-win) 
                        (eq (current-buffer) (window-buffer test-win)))
                     (window-list 
                      (car 
                       (remove-if
                        '(lambda (frm)
                           (eq (frame-selected-window frm) (selected-window)))
                        (frame-list)))))) 
                (point))))

;             (message (point))))
;             (message (format "point %d" (point)))))
;             (set-window-point (cadr (window-list)) (point))))
(require 'cl)
(require 'winsav)

(defun paddy-current-frame ()
  (window-configuration-frame (current-window-configuration)))
(defvar paddy-followed-frame (paddy-current-frame))
(defvar paddy-following-frame (make-frame))
(defun paddy-set-window-configuration (_winsav-wc _frame)

  (let ((_old-frame (paddy-current-frame)))
    (select-frame _frame)
    (delete-other-windows)
    (winsav-put-window-tree _wt (winsav-upper-left-window))
    (select-frame _old-frame)))
(add-hook 'window-or-frame-params-changing
          '(lambda ()
             (setf _wt (winsav-get-window-tree (paddy-current-frame)))
             (paddy-set-window-configuration 
              (winsav-get-window-tree (paddy-current-frame)) paddy-following-frame)))

(add-hook 'frame-size-changes
          '(lambda ()
             (set-frame-parameter paddy-following-frame  'width 
                                  (frame-parameter paddy-followed-frame 'width))
             (set-frame-parameter paddy-following-frame  'height
                                  (frame-parameter paddy-followed-frame 'height))))




(setf paddy-following-frame (make-frame))

