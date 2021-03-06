/*
 * Copyright (c) 2010, Fabian Greif
 * Copyright (c) 2015, Kevin Laeufer
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

/*
 * Created on 11.02.2004
 *
 * To change the template for this generated file go to
 * Window&gt;Preferences&gt;Java&gt;Code Generation&gt;Code and Comments
 */
package gui.util;

import javax.swing.table.DefaultTableModel;

/**
 * @author Fabian Maximilian Thiele
 */
public class CharTableModel extends DefaultTableModel {
	private static final long serialVersionUID = -1405657517916239473L;

	public CharTableModel() {
		super();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see javax.swing.table.DefaultTableModel#isCellEditable(int, int)
	 */
	public boolean isCellEditable(int row, int column) {
		if (column < 3)
			return false;
		else
			return true;
	}

	public void removeAllRows() {
		int cnt = getRowCount();
		for (int i = 0; i < cnt; i++) {
			removeRow(0);
		}
	}
}
